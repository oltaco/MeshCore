#include <nrf.h>
#include "CrashRecovery.h"

#define RESET_TRACKER_MAGIC    0xDEADFEED
#define OP_CODE_MASK           0x000000FF  // Bits 0-7
#define CRASH_COUNTER_MASK     0x00000F00  // Bits 8-11
#define CRASH_COUNTER_SHIFT    8

#define NOINIT_BASE 0x20006000 // start of noinit section, where we keep the reset tracker struct.

// NOINIT section structure
typedef struct {
    uint32_t magic_number;              // use magic number to verify validity
    uint32_t operation_and_counter;     // Bits 0-7: fs opcode, Bits 8-11: crash counter, Bits 12-31: reserved
    uint32_t reserved[30];              // so much room for activities!
} reset_tracker_t;

#define SET_OPERATION(val, op)          ((val & ~OP_CODE_MASK) | (op & 0xFF))
#define GET_OPERATION(val)              (val & OP_CODE_MASK)
#define SET_CRASH_COUNTER(val, count)   ((val & ~CRASH_COUNTER_MASK) | ((count & 0xF) << CRASH_COUNTER_SHIFT))
#define GET_CRASH_COUNTER(val)          ((val & CRASH_COUNTER_MASK) >> CRASH_COUNTER_SHIFT)


reset_tracker_t* reset_tracker = (reset_tracker_t*)NOINIT_BASE;


static void reset_tracker_init(void) {
    if (reset_tracker->magic_number != RESET_TRACKER_MAGIC) {
        MESH_DEBUG_PRINTLN("invalid number: 0x%08X, initializing reset_tracker with 0x%08X", reset_tracker->magic_number, RESET_TRACKER_MAGIC);
        memset((void*)reset_tracker, 0, sizeof(reset_tracker_t));
        reset_tracker->magic_number = RESET_TRACKER_MAGIC;
        reset_tracker->operation_and_counter = SET_OPERATION(0, OP_IDLE);
    }
}
static bool is_reset_tracker_valid(void) {
    return reset_tracker->magic_number == RESET_TRACKER_MAGIC;
}


void crashRecovery(FILESYSTEM& fs, FILESYSTEM& fsExtra) {
    FILESYSTEM* _fs = &fs;
    FILESYSTEM* _fsExtra = &fsExtra;
    uint32_t resetReason = readResetReason();

    reset_tracker_init();


    uint8_t operation = GET_OPERATION(reset_tracker->operation_and_counter);
    uint8_t resetCount = GET_CRASH_COUNTER(reset_tracker->operation_and_counter);

    MESH_DEBUG_PRINTLN("CrashRecovery: reset reason: 0x%08X", resetReason);
    if (resetReason == 0x01) {
        MESH_DEBUG_PRINTLN("ResetTracker: Pin-reset detected, clearing crash counter.");
        resetCount = 0;
        setCrashCount(resetCount);
    }
    
    if (operation != OP_IDLE && operation != OP_RECOVERY) {
        MESH_DEBUG_PRINTLN("CrashRecovery: Previous operation was 0x%02X, indicating a possible crash during filesystem operation.", operation);
        resetCount = (resetCount + 1) & 0x0F;
        setCrashCount(resetCount);
    }

    MESH_DEBUG_PRINTLN("CrashRecovery: reset_tracker: 0x%08X (operation: 0x%02X, resetCount: %d)", reset_tracker->operation_and_counter, operation, resetCount);

    switch (operation) {
        case OP_IDLE:
            MESH_DEBUG_PRINTLN("CrashRecovery: OP_IDLE, no crash detected");
            break;
        case OP_RECOVERY:
            MESH_DEBUG_PRINTLN("CrashRecovery: First boot after succesful recovery operation, resetting crash counter.");
            resetCount = 0;
            setCrashCount(resetCount);
            break;
        case OP_USERDATA_MOUNT:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from USERDATA_MOUNT...");
            if (resetCount >= 2) {
                _fs->format();
                MESH_DEBUG_PRINTLN("CrashRecovery: USERDATA_MOUNT recovery exceeded max retries, formatted filesystem.");
                setFSOpCode(OP_RECOVERY);
                delay(1000);
                NVIC_SystemReset();
            }
            break;
        case OP_CONTACTFS_MOUNT:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from CONTACTFS_MOUNT...");
            // Attempt to remount filesystem or take other recovery actions
            if (resetCount >= 2) {
                _fsExtra->format();
                MESH_DEBUG_PRINTLN("CrashRecovery: CONTACTFS_MOUNT: recovery exceeded max retries, formatted filesystem.");
                setFSOpCode(OP_RECOVERY);
                delay(1000);
                NVIC_SystemReset();
            }

            break;
        case OP_PREFS_READ:
        MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from PREFS_READ operation...");
            // TODO: Handle prefs file operation recovery
            break;
        case OP_PREFS_WRITE:
        MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from PREFS_WRITE operation...");
            // TODO: Handle prefs file operation recovery
            break;
        case OP_PREFS_REMOVE:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from PREFS operation...");
            // TODO: Handle prefs file operation recovery
            break;
        case OP_CONTACTS_READ:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from CONTACTS_READ operation...");
            // Handle contacts file operation recovery
            if (resetCount >= 4) {
                MESH_DEBUG_PRINTLN("CrashRecovery: CONTACTS_READ recovery exceeded max retries, forcing format.");
                _fsExtra->format();
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            } else if (resetCount >= 2) {
                MESH_DEBUG_PRINTLN("CrashRecovery: CONTACTS_READ recovery retry, removing contacts3 file.");
                setFSOpCode(OP_CONTACTS_REMOVE);
                int err = _fsExtra->remove("/contacts3");
                if (err == LFS_ERR_CORRUPT) {
                    MESH_DEBUG_PRINTLN("CrashRecovery: LFS_ERR_CORRUPT when attempting to remove contacts3, forcing format.");
                    _fsExtra->format();
                }
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            }
            break;
        case OP_CONTACTS_WRITE:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from CONTACTS_WRITE operation...");
            // Handle contacts file operation recovery
            if (resetCount >= 4) {
                MESH_DEBUG_PRINTLN("CrashRecovery: OP_CONTACTS_WRITE recovery exceeded max retries, forcing format.");
                _fsExtra->format();
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            } else if (resetCount >= 2) {
                MESH_DEBUG_PRINTLN("CrashRecovery: OP_CONTACTS_WRITE recovery retry, removing contacts3 file.");
                setFSOpCode(OP_CONTACTS_REMOVE);
                int err = _fsExtra->remove("/contacts3");
                if (err == LFS_ERR_CORRUPT) {
                    MESH_DEBUG_PRINTLN("CrashRecovery: LFS_ERR_CORRUPT when attempting to remove contacts3, forcing format.");
                    _fsExtra->format();
                }
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            }
            break;
        case OP_CONTACTS_REMOVE:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from CONTACTS_REMOVE operation...");
            // Handle contacts file operation recovery
            MESH_DEBUG_PRINTLN("CrashRecovery: Error while trying to remove contacts3, forcing format.");
            _fsExtra->format();
            setFSOpCode(OP_RECOVERY);
            NVIC_SystemReset();
            break;
        case OP_CHANNELS_READ:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from CHANNELS_READ operation...");
            // Handle channels file operation recovery
            if (resetCount >= 4) {
                MESH_DEBUG_PRINTLN("CrashRecovery: CHANNELS_READ recovery exceeded max retries, forcing format.");
                _fsExtra->format();
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            } else if (resetCount >= 2) {
                setFSOpCode(OP_CHANNELS_REMOVE);
                int err = _fsExtra->remove("/channels2");
                if (err == LFS_ERR_CORRUPT) {
                    MESH_DEBUG_PRINTLN("CrashRecovery: LFS_ERR_CORRUPT when attempting to remove channels2, forcing format.");
                    _fsExtra->format();
                }
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            }
            break;
        case OP_CHANNELS_WRITE:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from CHANNELS_WRITE operation...");
            // Handle channels file operation recovery
            if (resetCount >= 4) {
                MESH_DEBUG_PRINTLN("CrashRecovery: CHANNELS_WRITE recovery exceeded max retries, forcing format.");
                _fsExtra->format();
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            } else if (resetCount >= 2) {
                setFSOpCode(OP_CHANNELS_REMOVE);
                int err = _fsExtra->remove("/channels2");
                if (err == LFS_ERR_CORRUPT) {
                    MESH_DEBUG_PRINTLN("CrashRecovery: LFS_ERR_CORRUPT when attempting to remove channels2, forcing format.");
                    _fsExtra->format();
                }
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            }
            break;
        case OP_CHANNELS_REMOVE:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from CHANNELS_REMOVE operation...");
            // Handle channels file operation recovery
                MESH_DEBUG_PRINTLN("CrashRecovery: Error while trying to remove channels2, forcing format.");
                _fsExtra->format();
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            break;
        case OP_ADV_BLOBS_READ:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from ADV_BLOBS_READ operation...");
            // Handle adv_blobs file operation recovery
            if (resetCount >= 4) {
                MESH_DEBUG_PRINTLN("CrashRecovery: ADV_BLOBS_READ recovery exceeded max retries, forcing format.");
                _fsExtra->format();
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();    
            } else if (resetCount >= 2) {
                setFSOpCode(OP_ADV_BLOBS_REMOVE);
                int err = _fsExtra->remove("/adv_blobs");
                if (err == LFS_ERR_CORRUPT) {
                    MESH_DEBUG_PRINTLN("CrashRecovery: LFS_ERR_CORRUPT when attempting to remove adv_blobs, forcing format.");
                    _fsExtra->format();
                }
                delay(1000);                
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            }
            break;
        case OP_ADV_BLOBS_WRITE:
            MESH_DEBUG_PRINTLN("CrashRecovery: Entering ADV_BLOBS_WRITE operation...");
            // Handle adv_blobs file operation recovery
            if (resetCount >= 2) {
                MESH_DEBUG_PRINTLN("CrashRecovery: ADV_BLOBS_WRITE recovery exceeded max retries, forcing format.");
                _fsExtra->format();
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            } else if (resetCount >= 1) {
                MESH_DEBUG_PRINTLN("CrashRecovery: ADV_BLOBS_WRITE recovery: removing adv_blobs file.");
                setFSOpCode(OP_ADV_BLOBS_REMOVE);
                int err = _fsExtra->remove("/adv_blobs");
                MESH_DEBUG_PRINTLN("CrashRecovery: ADV_BLOBS_WRITE recovery: remove returned %d", err);
                // if (_fsExtra->remove("/adv_blobs") == LFS_ERR_CORRUPT) {
                //     MESH_DEBUG_PRINTLN("CrashRecovery: LFS_ERR_CORRUPT when attempting to remove adv_blobs, forcing format.");
                //     _fsExtra->format();
                // }
                delay(1000);
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            }
            break;
        case OP_ADV_BLOBS_REMOVE:
            MESH_DEBUG_PRINTLN("CrashRecovery: Recovering from OP_ADV_BLOBS_REMOVE operation...");
            // Handle adv_blobs file operation recovery
                MESH_DEBUG_PRINTLN("CrashRecovery: Error trying to remove adb_blobs, forcing format.");
                _fsExtra->format();
                setFSOpCode(OP_RECOVERY);
                NVIC_SystemReset();
            break;
        default:
            MESH_DEBUG_PRINTLN("CrashRecovery: Unknown operation code: 0x%02X", operation);
            break;
    }

    MESH_DEBUG_PRINTLN("CrashRecovery: Recovery not attempted.");
    setFSOpCode(OP_IDLE);
    MESH_DEBUG_PRINTLN("CrashRecovery: reset_tracker: 0x%08X (operation: 0x%02X, resetCount: %d)", reset_tracker->operation_and_counter, operation, resetCount);

}

void setFSOpCode(fs_opcode operation) {
    reset_tracker->magic_number = RESET_TRACKER_MAGIC;
    reset_tracker->operation_and_counter = SET_OPERATION(reset_tracker->operation_and_counter, operation);
    __DSB(); // Add sync barrier to ensure memory writes complete before a reset occurs
}

uint8_t getFSOpCode() {
    if (!is_reset_tracker_valid()) {
        reset_tracker_init();
        return OP_IDLE;
    }
    
    return (fs_opcode)GET_OPERATION(reset_tracker->operation_and_counter);
}

uint8_t getCrashCount() {
    if (!is_reset_tracker_valid()) {
        reset_tracker_init();
        return 0;
    }
    
    return GET_CRASH_COUNTER(reset_tracker->operation_and_counter);
}

void setCrashCount(uint8_t count) {
    if (!is_reset_tracker_valid()) {
        reset_tracker_init();
    }
    
    uint8_t currentOp = GET_OPERATION(reset_tracker->operation_and_counter);
    
    // Combine preserved operation with new counter
    reset_tracker->operation_and_counter = SET_CRASH_COUNTER(reset_tracker->operation_and_counter, count & 0x0F);
    
    // Add sync barrier to ensure memory writes complete before a reset occurs
    __DSB();
}
