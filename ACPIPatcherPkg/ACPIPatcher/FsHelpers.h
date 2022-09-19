/** @file

  File system helper functions.

  By dmazar, 26/09/2012
     jslegendre, 17/04/2019

**/

#ifndef __FILE_LIB_H__
#define __FILE_LIB_H__

#include <Protocol/LoadedImage.h>

/*++
 
 Routine Description:
 
 Open file in provided directory.
 
 Arguments:
 
 Directory       - User-provided directory handle/protocol to read file from.
 FileName        - Name of file to open.
 FileProtocol    - EFI_FILE_PROTOCOL of opened file.
 
 Returns: EFI_STATUS
 
 --*/
EFI_STATUS
FsOpenFile (
  IN        EFI_FILE_PROTOCOL   *Directory,
  IN        CHAR16*             FileName,
  IN OUT    EFI_FILE_PROTOCOL   **FileProtocol
  );

/*++
 
 Routine Description:
 
 Reads open file handle/protocol to user-provided buffer.
 
 Arguments:
 
 FileProtocol      - User-provided file handle/protocol to read.
 BufferSize        - Size of FileProtocol file.
 Buffer            - Buffer to read file into. Allocated here and should be released by caller
 
 Returns: EFI_STATUS
 
 --*/
EFI_STATUS
FsReadFileToBuffer (
  IN      EFI_FILE_PROTOCOL   *FileProtocol,
  IN      UINTN               BufferSize,
  IN OUT  VOID                **Buffer
  );

/** Returns file path from FilePathProto in allocated memory. Mem should be released by caller.*/
CHAR16 *
EFIAPI
FileDevicePathToText(EFI_DEVICE_PATH_PROTOCOL *FilePathProto);

/** Retrieves loaded image protocol from our image. */
VOID
FsGetLoadedImage(VOID);

/** Returns file system protocol from specified volume device. */
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
FsGetFileSystem(IN EFI_HANDLE VolumeHandle);

/** Returns file system protocol from volume device we are loaded from. */
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
FsGetSelfFileSystem(VOID);

/** Returns root dir from given file system. */
EFI_FILE_PROTOCOL *
FsGetRootDir(IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume);

/** Returns root dir file from file system we are loaded from. */
EFI_FILE_PROTOCOL *
FsGetSelfRootDir(VOID);

/** Returns dir from file system we are loaded from. */
EFI_FILE_PROTOCOL *
FsGetSelfDir(VOID);

#endif // __DMP_FILE_LIB_H__
