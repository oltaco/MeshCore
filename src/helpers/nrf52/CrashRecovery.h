typedef enum {
    OP_IDLE = 0x00,
    // Mount operations
    OP_USERDATA_MOUNT = 0x01,
    OP_CONTACTFS_MOUNT = 0x02,
    // Prefs file operations
    OP_PREFS_READ = 0x03,
    OP_PREFS_WRITE = 0x04,
    OP_PREFS_REMOVE = 0x05,
    // Contacts file operations
    OP_CONTACTS_READ = 0x06,
    OP_CONTACTS_WRITE = 0x07,
    OP_CONTACTS_REMOVE = 0x08,
    // Channels file operations
    OP_CHANNELS_READ = 0x09,
    OP_CHANNELS_WRITE = 0x0A,
    OP_CHANNELS_REMOVE = 0x0B,
    // Adv_blobs file operations
    OP_ADV_BLOBS_READ = 0x0C,
    OP_ADV_BLOBS_WRITE = 0x0D,
    OP_ADV_BLOBS_REMOVE = 0x0E,
    // recovery ops
    OP_RECOVERY = 0x0F,
} fs_opcode;

// void crashRecovery();
void setFSOpCode(fs_opcode operation);
uint8_t getFSOpCode();
uint8_t getCrashCount();
void setCrashCount(uint8_t count);