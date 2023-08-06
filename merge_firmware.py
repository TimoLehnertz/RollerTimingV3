Import("env")

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
MERGED_BIN = "$BUILD_DIR/${PROGNAME}_merged.bin"
BOARD_CONFIG = env.BoardConfig()


def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]

    # Run esptool to merge images into a single binary
    # python -m esptool --chip ESP32 merge_bin -o merged-flash.bin --flash_mode dio --flash_size 4MB 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
    # env.Execute("$PYTHONEXE -m esptool --chip ESP32s3 merge_bin -o " + MERGED_BIN + " --flash_mode dio --flash_size 4MB 0x1000 $BUILD_DIR/bootloader.bin 0x8000 $BUILD_DIR/partitions.bin 0x10000 $BUILD_DIR/firmware.bin")
    env.Execute("cd $BUILD_DIR")
    env.Execute("python -m esptool --chip ESP32s3 merge_bin -o merged-flash.bin --flash_mode dio --flash_size 8MB 0x1000 $BUILD_DIR/bootloader.bin 0x8000 $BUILD_DIR/partitions.bin 0x10000 $BUILD_DIR/firmware.bin")
    env.Execute("cd ../../../")
    
    # env.Execute(
    #     " ".join(
    #         [
    #             "$PYTHONEXE",
    #             "$OBJCOPY",
    #             "--chip",
    #             BOARD_CONFIG.get("build.mcu", "esp32"),
    #             "merge_bin",
    #             "-o",
    #             MERGED_BIN,
    #             "--flash_mode dio",
    #             "dio",
    #             "--fill-flash-size",
    #             BOARD_CONFIG.get("upload.flash_size", "4MB"),
    #         ]
    #         + flash_images
    #     )
    # )

# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction(APP_BIN , merge_bin)

# Patch the upload command to flash the merged binary at address 0x0
# env.Replace(
#     UPLOADERFLAGS=[
#         ]
#         + ["0x0", MERGED_BIN],
#     UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS',
# )