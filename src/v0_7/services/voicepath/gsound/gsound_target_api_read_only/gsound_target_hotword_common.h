// Copyright 2019 Google LLC.
// Libgsound version: f5d8a0a
#ifndef GSOUND_TARGET_HOTWORD_MODEL_COMMON_H
#define GSOUND_TARGET_HOTWORD_MODEL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file is used for all targets implementing hotword, where the hotword
 * detection is handled by GSound (internal) or by the target (external).
 */

#define SUPPORTED_HOTWORD_MODEL_DELIM "\n"

#define GSOUND_HOTWORD_MODEL_ID_BYTES 4

typedef enum {
  /**
   * An mmap of this type is Read-Only data
   */
  GSOUND_HOTWORD_MMAP_TEXT,
  /*
   * An mmap of this type is preinitialized Read-Write data
   */
  GSOUND_HOTWORD_MMAP_DATA,
  /*
   * An mmap of this type is uninitialized Read-Write data
   */
  GSOUND_HOTWORD_MMAP_BSS
} GSoundHotwordMmapType;

#ifdef __cplusplus
}
#endif

#endif  // GSOUND_TARGET_HOTWORD_MODEL_COMMON_H
