/** @file

  File system helper functions.

  By dmazar, 26/09/2012
     jslegendre, 17/04/2019

**/


#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseLib.h>

#include <Protocol/LoadedImage.h>

#include <Guid/Gpt.h>

#include "FsHelpers.h"
EFI_LOADED_IMAGE_PROTOCOL           *gLoadedImage;

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
  EFI_FILE_PROTOCOL* FileProtocol,
  UINTN BufferSize,
  VOID ** Buffer
  )
{
    EFI_STATUS Status = EFI_SUCCESS;
    *Buffer = AllocateZeroPool(BufferSize);
    Status = FileProtocol->Read(FileProtocol, &BufferSize, *Buffer);
    if(Status != EFI_SUCCESS) {
        return Status;
    }
    
    return Status;
}

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
  EFI_FILE_PROTOCOL* Directory,
  CHAR16* FileName,
  EFI_FILE_PROTOCOL** FileProtocol
  )
{
    EFI_STATUS Status = EFI_SUCCESS;
    Status = Directory->Open(Directory,
                             FileProtocol,
                             FileName,
                             EFI_FILE_MODE_READ,
                             EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    
    return Status;
}

/** Returns file path from FilePathProto in allocated memory. Mem should be released by caler.*/
CHAR16 *
EFIAPI
FileDevicePathToText(EFI_DEVICE_PATH_PROTOCOL *FilePathProto)
{
    EFI_STATUS              Status;
    FILEPATH_DEVICE_PATH    *FilePath;
    CHAR16                  FilePathText[256]; // possible problem: if filepath is bigger
    CHAR16                  *OutFilePathText;
    INTN                    Size;
    INTN                    SizeAll;
    INTN                    i;
    
    FilePathText[0] = L'\0';
    i = 4;
    SizeAll = 0;
    while (FilePathProto != NULL && FilePathProto->Type != END_DEVICE_PATH_TYPE && i > 0) {
        if (FilePathProto->Type == MEDIA_DEVICE_PATH && FilePathProto->SubType == MEDIA_FILEPATH_DP) {
            FilePath = (FILEPATH_DEVICE_PATH *) FilePathProto;
            Size = (DevicePathNodeLength(FilePathProto) - 4) / 2;
            if (SizeAll + Size < 256) {
                if (SizeAll > 0 && FilePathText[SizeAll / 2 - 2] != L'\\') {
                    StrCatS(FilePathText, 256, L"\\");
                }
                StrCatS(FilePathText, 256, FilePath->PathName);
                SizeAll = StrSize(FilePathText);
            }
        }
        FilePathProto = NextDevicePathNode(FilePathProto);
        i--;
    }
    
    OutFilePathText = NULL;
    Size = StrSize(FilePathText);
    if (Size > 2) {
        // we are allocating mem here - should be released by caller
        Status = gBS->AllocatePool(EfiBootServicesData, Size, (VOID*)&OutFilePathText);
        if (Status == EFI_SUCCESS) {
            StrCpyS(OutFilePathText, Size/sizeof(CHAR16), FilePathText);
        } else {
            OutFilePathText = NULL;
        }
    }
    
    return OutFilePathText;
}

/** Retrieves loaded image protocol from our image. */
VOID
FsGetLoadedImage(VOID)
{
	EFI_STATUS			Status;
	
	
	if (gLoadedImage == NULL) {
		// get our EfiLoadedImageProtocol
		Status = gBS->HandleProtocol(
			gImageHandle,
			&gEfiLoadedImageProtocolGuid,
			(VOID **) &gLoadedImage
			);
		
		if (Status != EFI_SUCCESS) {
			Print(L"FsGetLoadedImage: HandleProtocol(gEfiLoadedImageProtocolGuid) = %r\n", Status);
			return;
		}
	}
}

/** Returns file system protocol from specified volume device. */
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
FsGetFileSystem(IN EFI_HANDLE VolumeHandle)
{
	EFI_STATUS						Status;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL	*Volume;
	
	
	// open EfiSimpleFileSystemProtocol from device
	Status = gBS->HandleProtocol(
								 VolumeHandle,
								 &gEfiSimpleFileSystemProtocolGuid,
								 (VOID **) &Volume
								 );
	
	if (Status != EFI_SUCCESS) {
		Print(L"FsGetFileSystem: HandleProtocol(gEfiSimpleFileSystemProtocolGuid) = %r\n", Status);
		Volume = NULL;
	}
	
	return Volume;
}

/** Returns file system protocol from volume device we are loaded from. */
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *
FsGetSelfFileSystem(VOID)
{
	
	FsGetLoadedImage();
	if (gLoadedImage == NULL) {
		return NULL;
	}
	
	return FsGetFileSystem(gLoadedImage->DeviceHandle);
}

/** Returns root dir from given file system. */
EFI_FILE_PROTOCOL *
FsGetRootDir(IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume)
{
	EFI_STATUS			Status;
	EFI_FILE_PROTOCOL	*RootDir;
	
	
	if (Volume == NULL) {
		return NULL;
	}
	
	// open RootDir
	Status = Volume->OpenVolume(Volume, &RootDir);
	if (Status != EFI_SUCCESS) {
		Print(L"FsGetRootDir: OpenVolume() = %r\n", Status);
		return NULL;
	}
	
	return RootDir;
}

/** Returns root dir file from file system we are loaded from. */
EFI_FILE_PROTOCOL *
FsGetSelfRootDir(VOID)
{
	
	return FsGetRootDir(FsGetSelfFileSystem());
}

/** Returns dir from file system we are loaded from. */
EFI_FILE_PROTOCOL *
FsGetSelfDir(VOID)
{
	EFI_STATUS			Status;
	EFI_FILE_PROTOCOL	*RootDir;
	EFI_FILE_PROTOCOL	*File;
	EFI_FILE_PROTOCOL	*Dir;
	CHAR16				*FilePath;
	CHAR16				*DirName;
	UINTN				Index;
	
	
	// make sure we have our loaded image protocol
	FsGetLoadedImage();
	if (gLoadedImage == NULL) {
		return NULL;
	}
	
	RootDir = FsGetSelfRootDir();
	if (RootDir == NULL) {
		return NULL;
	}
	
	// extract FilePath
	FilePath = FileDevicePathToText(gLoadedImage->FilePath);
	if (FilePath == NULL) {
		Print(L"FsGetSelfDir: FileDevicePathToText = NULL\n");
		return NULL;
	}
	
	// open file
	Status = RootDir->Open(RootDir, &File, FilePath, EFI_FILE_MODE_READ, 0);
	RootDir->Close(RootDir);
	if (Status != EFI_SUCCESS) {
		Print(L"FsGetSelfDir: Open(%s) = %r\n", FilePath, Status);
		FreePool(FilePath);
		return NULL;
	}
	  
	// find parent dir by putting \0 to last \\ in file path
	for (Index = StrLen(FilePath); Index > 0 && FilePath[Index] != '\\'; Index--) {
		;
	}
	if (Index > 0) {
		FilePath[Index] = L'\0';
		DirName = FilePath;
	} else {
		DirName = L"\\";
	}

	Status = File->Open(File, &Dir, DirName, EFI_FILE_MODE_READ, 0);
	File->Close(File);
	if (Status != EFI_SUCCESS) {
		Print(L"FsGetSelfDir: Open(%s) = %r\n", DirName, Status);
		FreePool(FilePath);
		return NULL;
	}
	FreePool(FilePath);
	
	return Dir;
}
