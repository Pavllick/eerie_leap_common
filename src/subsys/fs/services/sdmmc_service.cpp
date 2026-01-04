#include <filesystem>

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>
#ifdef CONFIG_LOG_BACKEND_FS
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_fs.h>
#endif // CONFIG_LOG_BACKEND_FS

#include "sdmmc_service.h"

namespace eerie_leap::subsys::fs::services {

LOG_MODULE_REGISTER(sdmmc_service_logger);

SdmmcService::SdmmcService(fs_mount_t mountpoint, const char* disk_name)
    : FsService(mountpoint), disk_name_(disk_name), monitor_running_(ATOMIC_INIT(0)) {

    thread_ = std::make_unique<Thread>(
        "sd_monitor", this, k_stack_size_, k_priority_, true);
}

SdmmcService::~SdmmcService() {
    thread_->Join();
}

bool SdmmcService::Initialize() {
    sd_card_present_ = FsService::Initialize();
    if(!sd_card_present_)
        return false;

    PrintInfo();
    return true;
}

void SdmmcService::RegisterIsSdCardPresentHandler(std::function<bool()> handler) {
    _is_sd_card_present_handler = handler;
}

bool SdmmcService::IsAvailable() const {
    return sd_card_present_;
}

bool SdmmcService::IsSdCardAttached(const char* disk_name) {
    bool bool_true = true;
    disk_access_ioctl(disk_name, DISK_IOCTL_CTRL_DEINIT, &bool_true);

    return disk_access_ioctl(disk_name, DISK_IOCTL_CTRL_INIT, nullptr) == 0;
}

void SdmmcService::SdMonitorHandler() {
    if(_is_sd_card_present_handler == nullptr)
        return;

    bool card_detected = _is_sd_card_present_handler();

    if(!card_detected) {
        IsSdCardAttached(disk_name_);
    }

    if(card_detected != sd_card_present_) {
        sd_card_present_ = card_detected;

        if(card_detected) {
            LOG_INF("SD card detected.");

            Unmount();
            if(!Mount()) {
                sd_card_present_ = false;
            }
        } else {
            LOG_INF("SD card removed.");
        }
    }

#ifdef CONFIG_LOG_BACKEND_FS
    if(sd_card_present_ && log_backend_fs_get_state() == BACKEND_FS_CORRUPTED)
        log_backend_init(log_backend_fs_get());
#endif // CONFIG_LOG_BACKEND_FS
}

void SdmmcService::ThreadEntry() {
    while(atomic_get(&monitor_running_)) {
        SdMonitorHandler();

        if(k_sem_take(&sd_monitor_stop_sem_, K_MSEC(CONFIG_EERIE_LEAP_SD_CHECK_INTERVAL_MS)) == 0)
            break;
    }
}

int SdmmcService::SdMonitorStart() {
    k_sem_init(&sd_monitor_stop_sem_, 0, 1);
    atomic_set(&monitor_running_, 1);

    thread_->Initialize();
    thread_->Start();

    LOG_INF("SD card monitoring started.");

    return 0;
}

int SdmmcService::SdMonitorStop() {
    if(!atomic_get(&monitor_running_))
        return 0;

    atomic_set(&monitor_running_, 0);
    k_sem_give(&sd_monitor_stop_sem_);
    thread_->Join();

    LOG_INF("SD card monitoring stopped.");

    return 0;
}

int SdmmcService::PrintInfo() const {
    uint64_t memory_size_mb = 0;
    uint32_t block_count = 0;
    uint32_t block_size = 0;

    int ret = disk_access_ioctl(disk_name_, DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
    if(ret != 0) {
        LOG_ERR("Unable to get sector count (%d)", ret);
        return ret;
    }

    ret = disk_access_ioctl(disk_name_, DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
    if(ret != 0) {
        LOG_ERR("Unable to get sector size (%d)", ret);
        return ret;
    }

    memory_size_mb = (uint64_t)block_count * block_size / (1024 * 1024);
    LOG_INF("SD card: %llu MB, %u sectors, %u bytes/sector",
        memory_size_mb, block_count, block_size);

    return 0;
}

} // namespace eerie_leap::subsys::fs::services
