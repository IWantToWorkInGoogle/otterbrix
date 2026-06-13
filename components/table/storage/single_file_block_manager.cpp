#include "single_file_block_manager.hpp"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdexcept>

#include <absl/crc/crc32c.h>
#include <components/table/storage/metadata_manager.hpp>
#include <components/table/storage/metadata_reader.hpp>
#include <components/table/storage/metadata_writer.hpp>
#include <core/file/file_handle.hpp>
#include <core/file/local_file_system.hpp>

namespace components::table::storage {

    namespace {
        // CRC32c over the meaningful header fields (everything before the checksum field); the
        // checksum field itself and the trailing padding are excluded. A torn/partial header write or
        // a flipped field then fails this check on load instead of being silently accepted as the
        // active header (which would steer recovery into a garbage meta_block).
        uint64_t compute_header_checksum(const database_header_t& header) {
            return static_cast<uint64_t>(static_cast<uint32_t>(absl::ComputeCrc32c(
                {reinterpret_cast<const char*>(&header), offsetof(database_header_t, checksum)})));
        }
    } // namespace

    single_file_block_manager_t::single_file_block_manager_t(buffer_manager_t& buffer_manager,
                                                             core::filesystem::local_file_system_t& fs,
                                                             const std::string& path,
                                                             uint64_t block_alloc_size)
        : block_manager_t(buffer_manager, block_alloc_size)
        , fs_(fs)
        , path_(path) {}

    single_file_block_manager_t::~single_file_block_manager_t() = default;

    uint64_t single_file_block_manager_t::block_location(uint64_t block_id) const {
        return BLOCK_START + block_id * block_allocation_size();
    }

    // --- Database lifecycle ---

    void single_file_block_manager_t::create_new_database() {
        using namespace core::filesystem;

        handle_ = open_file(fs_,
                            path_,
                            file_flags::WRITE | file_flags::READ | file_flags::FILE_CREATE_NEW,
                            file_lock_type::WRITE_LOCK);
        if (!handle_) {
            throw std::runtime_error("Failed to create database file: " + path_);
        }

        main_header_t main_header;
        main_header.initialize();
        if (!handle_->write(&main_header, sizeof(main_header), 0)) {
            throw std::runtime_error("create_new_database: failed to write main header");
        }

        database_header_t db_header;
        db_header.initialize();
        db_header.block_alloc_size = block_allocation_size();
        // Stamp the integrity checksum on the initial header too, or a create-then-reopen (no
        // checkpoint in between) would fail header validation on load.
        db_header.checksum = compute_header_checksum(db_header);
        write_header_slot(db_header, SECTOR_SIZE);
        write_header_slot(db_header, 2 * SECTOR_SIZE);

        iteration_ = 0;
        max_block_ = 0;
        meta_block_ = INVALID_INDEX;
    }

    void single_file_block_manager_t::load_existing_database() {
        using namespace core::filesystem;

        handle_ = open_file(fs_,
                            path_,
                            file_flags::WRITE | file_flags::READ | file_flags::FILE_CREATE,
                            file_lock_type::WRITE_LOCK);
        if (!handle_) {
            throw std::runtime_error("Failed to open database file: " + path_);
        }

        main_header_t main_header;
        if (!handle_->read(&main_header, sizeof(main_header), 0)) {
            throw std::runtime_error("Failed to read main header");
        }
        if (!main_header.validate()) {
            throw std::runtime_error("Invalid database file: bad magic or version");
        }

        database_header_t header1, header2;
        if (!handle_->read(&header1, sizeof(header1), SECTOR_SIZE)) {
            throw std::runtime_error("Failed to read database header 1");
        }
        if (!handle_->read(&header2, sizeof(header2), 2 * SECTOR_SIZE)) {
            throw std::runtime_error("Failed to read database header 2");
        }

        // Reject a slot whose integrity checksum does not match before selecting the active header.
        // A torn header write leaves one slot inconsistent; recovering from the other intact slot
        // (instead of promoting the torn one by raw iteration) is the point of the double-header.
        const bool valid1 = header1.checksum == compute_header_checksum(header1);
        const bool valid2 = header2.checksum == compute_header_checksum(header2);
        if (!valid1 && !valid2) {
            throw std::runtime_error(
                "load_existing_database: both database header slots failed checksum (torn or corrupt header)");
        }
        const database_header_t& active = (valid1 && valid2)
                                              ? ((header1.iteration >= header2.iteration) ? header1 : header2)
                                              : (valid1 ? header1 : header2);

        iteration_ = active.iteration;
        meta_block_ = active.meta_block;
        max_block_ = active.block_count;

        if (active.block_alloc_size != 0 && active.block_alloc_size != block_allocation_size()) {
            set_block_allocation_size(active.block_alloc_size);
        }

        if (active.free_list != INVALID_INDEX) {
            deserialize_free_list(meta_block_pointer_t{active.free_list, 0});
        }
    }

    // --- Block I/O ---

    void single_file_block_manager_t::read(block_t& block) {
        auto location = block_location(block.id);
        block.read(*handle_, location);

        if (!verify_checksum(block)) {
            throw std::runtime_error("Block checksum mismatch for block " + std::to_string(block.id));
        }
    }

    void single_file_block_manager_t::read_blocks(file_buffer_t& buffer, uint64_t start_block, uint64_t block_count) {
        auto location = block_location(start_block);
        buffer.read(*handle_, location);

        // Verify every block's CRC32c. The single-block read() path already does this, but the
        // batch/prefetch path (the primary cold-reopen read path for PAX *data* blocks) used to
        // skip it — a corrupted data block was then scanned silently and returned wrong results
        // instead of failing. Metadata blocks (read via read()) were protected; data blocks were
        // not. Blocks sit at block_allocation_size() stride in the buffer (same stride batch_read
        // slices payloads with); each block is [8-byte checksum][block_size() payload], matching
        // checksum_and_write (payload = allocation_size - 8 = block_size()). Guard the stride so a
        // smaller-than-expected buffer can never overrun.
        auto* base = buffer.internal_buffer();
        const auto stride = block_allocation_size();
        const auto payload_size = block_size();
        for (uint64_t i = 0; i < block_count; i++) {
            if ((i + 1) * stride > buffer.allocation_size()) {
                break;
            }
            auto* block_ptr = base + i * stride;
            auto stored_checksum = *reinterpret_cast<uint64_t*>(block_ptr);
            auto computed = static_cast<uint64_t>(static_cast<uint32_t>(absl::ComputeCrc32c(
                {reinterpret_cast<const char*>(block_ptr + sizeof(uint64_t)), payload_size})));
            if (stored_checksum != computed) {
                throw std::runtime_error("Block checksum mismatch for block " + std::to_string(start_block + i));
            }
        }
    }

    void single_file_block_manager_t::write(file_buffer_t& buffer, uint64_t block_id) {
        checksum_and_write(buffer, block_id);
    }

    // --- Block allocation ---

    uint64_t single_file_block_manager_t::free_block_id() {
        std::lock_guard lock(allocation_lock_);
        uint64_t block_id;
        if (!free_list_.empty()) {
            auto it = free_list_.begin();
            block_id = *it;
            free_list_.erase(it);
        } else {
            block_id = max_block_++;
        }
        used_blocks_.insert(block_id);
        return block_id;
    }

    uint64_t single_file_block_manager_t::peek_free_block_id() {
        std::lock_guard lock(allocation_lock_);
        if (!free_list_.empty()) {
            return *free_list_.begin();
        }
        return max_block_;
    }

    bool single_file_block_manager_t::is_root_block(meta_block_pointer_t root) {
        return root.block_pointer == meta_block_;
    }

    void single_file_block_manager_t::mark_as_free(uint64_t block_id) {
        std::lock_guard lock(allocation_lock_);
        used_blocks_.erase(block_id);
        modified_blocks_.erase(block_id);
        free_list_.insert(block_id);
    }

    void single_file_block_manager_t::mark_as_used(uint64_t block_id) {
        std::lock_guard lock(allocation_lock_);
        free_list_.erase(block_id);
        used_blocks_.insert(block_id);
    }

    void single_file_block_manager_t::mark_as_modified(uint64_t block_id) {
        std::lock_guard lock(allocation_lock_);
        modified_blocks_.insert(block_id);
    }

    void single_file_block_manager_t::increase_block_ref_count(uint64_t /*block_id*/) {
        // ref counting not yet needed for single-file mode
    }

    uint64_t single_file_block_manager_t::meta_block() { return meta_block_; }

    std::unique_ptr<block_t> single_file_block_manager_t::create_block(uint64_t block_id,
                                                                       file_buffer_t* source_buffer) {
        auto& bm = buffer_manager;
        auto resource = bm.resource();

        if (source_buffer) {
            auto result = std::make_unique<block_t>(*source_buffer, block_id);
            return result;
        }
        return std::make_unique<block_t>(resource, block_id, static_cast<uint64_t>(block_size()));
    }

    std::unique_ptr<block_t> single_file_block_manager_t::convert_block(uint64_t block_id,
                                                                        file_buffer_t& source_buffer) {
        return std::make_unique<block_t>(source_buffer, block_id);
    }

    uint64_t single_file_block_manager_t::total_blocks() { return max_block_; }

    uint64_t single_file_block_manager_t::free_blocks() {
        std::lock_guard lock(allocation_lock_);
        return free_list_.size();
    }

    // --- Checksums ---

    void single_file_block_manager_t::checksum_and_write(file_buffer_t& buffer, uint64_t block_id) {
        auto* data = buffer.internal_buffer();
        auto alloc_size = buffer.allocation_size();

        // first 8 bytes = checksum slot
        auto* checksum_slot = reinterpret_cast<uint64_t*>(data);
        auto* payload = data + sizeof(uint64_t);
        auto payload_size = alloc_size - sizeof(uint64_t);

        auto crc = static_cast<uint64_t>(
            static_cast<uint32_t>(absl::ComputeCrc32c({reinterpret_cast<const char*>(payload), payload_size})));
        *checksum_slot = crc;

        auto location = block_location(block_id);
        buffer.write(*handle_, location);
    }

    bool single_file_block_manager_t::verify_checksum(file_buffer_t& buffer) {
        auto* data = buffer.internal_buffer();
        auto alloc_size = buffer.allocation_size();

        auto stored_checksum = *reinterpret_cast<uint64_t*>(data);
        auto* payload = data + sizeof(uint64_t);
        auto payload_size = alloc_size - sizeof(uint64_t);

        auto computed = static_cast<uint64_t>(
            static_cast<uint32_t>(absl::ComputeCrc32c({reinterpret_cast<const char*>(payload), payload_size})));
        return stored_checksum == computed;
    }

    // --- Header write + sync ---

    void single_file_block_manager_t::write_header(const database_header_t& header) {
        iteration_++;

        database_header_t write_header = header;
        write_header.iteration = iteration_;
        write_header.block_count = max_block_;
        write_header.block_alloc_size = block_allocation_size();
        write_header.meta_block = meta_block_;
        // Stamp the integrity checksum LAST, over the finalized fields. Validated on load; a torn or
        // bit-flipped header is then rejected rather than silently promoted.
        write_header.checksum = compute_header_checksum(write_header);

        // double-header protocol: alternate between slot 1 and slot 2
        uint64_t slot = (iteration_ % 2 == 1) ? SECTOR_SIZE : (2 * SECTOR_SIZE);
        write_header_slot(write_header, slot);

        // write to the other slot as well for redundancy
        uint64_t other_slot = (slot == SECTOR_SIZE) ? (2 * SECTOR_SIZE) : SECTOR_SIZE;
        write_header_slot(write_header, other_slot);
    }

    void single_file_block_manager_t::write_header_slot(const database_header_t& header, uint64_t slot) {
        // Propagate I/O failures: a discarded write()/sync() error would let a non-durable checkpoint
        // report success. write() and sync() return false on failure.
        if (!handle_->write(const_cast<database_header_t*>(&header), sizeof(header), slot)) {
            throw std::runtime_error("write_header: failed to write database header slot");
        }
        if (!handle_->sync()) {
            throw std::runtime_error("write_header: failed to fsync database header slot");
        }
    }

    void single_file_block_manager_t::file_sync() {
        if (handle_) {
            // Propagate fsync failure: this is the checkpoint commit's durability barrier (called
            // before and after the header swap). A discarded failure would let a non-durable
            // checkpoint report success — "committed" must mean the bytes reached stable storage.
            if (!handle_->sync()) {
                throw std::runtime_error("file_sync: fsync failed (checkpoint not durable)");
            }
        }
    }

    void single_file_block_manager_t::truncate() {
        if (handle_) {
            auto file_end = block_location(max_block_);
            handle_->truncate(static_cast<int64_t>(file_end));
        }
    }

    // --- Free List Persistence ---

    meta_block_pointer_t single_file_block_manager_t::serialize_free_list() {
        if (free_list_.empty()) {
            return meta_block_pointer_t{}; // INVALID_INDEX
        }
        metadata_manager_t meta_mgr(*this);
        metadata_writer_t writer(meta_mgr);
        writer.write<uint64_t>(free_list_.size());
        for (auto block_id : free_list_) {
            writer.write<uint64_t>(block_id);
        }
        writer.flush();
        return writer.get_block_pointer();
    }

    void single_file_block_manager_t::deserialize_free_list(meta_block_pointer_t pointer) {
        if (!pointer.is_valid()) {
            return;
        }
        metadata_manager_t meta_mgr(*this);
        metadata_reader_t reader(meta_mgr, pointer);
        auto count = reader.read<uint64_t>();
        for (uint64_t i = 0; i < count && !reader.finished(); ++i) {
            free_list_.insert(reader.read<uint64_t>());
        }
    }

} // namespace components::table::storage
