#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Guid/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>

EFI_BOOT_SERVICES 		  *gBS;
EFI_HANDLE 				      *gIH;
EFI_SHELL_PROTOCOL      *gEfiShellProtocol;

VOID *
NewBufferFromData (
  VOID * Source, 
  UINT8 length
  )
{
	VOID * Dest = AllocateZeroPool(length);
	CopyMem(Dest, Source, length);
	return Dest;
}

CHAR16 *
GetCurrentPath() 
{
 	EFI_SHELL_PARAMETERS_PROTOCOL *ShellParameters = NULL;

 	EFI_STATUS Status = gBS->HandleProtocol(gIH, 
 					&gEfiShellParametersProtocolGuid,
 	                (VOID**)&ShellParameters);

 	if (EFI_ERROR(Status)) {
 	  Print(L"No ShellParamProtocol\n");
 	}

  UINT32 Index = StrLen(ShellParameters->Argv[0]);

  while(Index != 0 && StrnCmp(&ShellParameters->Argv[0][Index], L"\\", 1)) {
    Index--;
  }

  if(!Index) {
    return NULL;
  }

  //Double Index because Unicode character is 2 bytes
  Index = (Index * 2) + 2;

  CHAR16 * path = (CHAR16 *)NewBufferFromData(ShellParameters->Argv[0], Index);

  //Add 'terminator' for consistency
  path[Index/2] = 0x0;
    return path;
}

EFI_STATUS
OpenACPIFolder (
  EFI_SHELL_FILE_INFO ** ACPIFolderHead
  ) 
{
  CHAR16 * path = GetCurrentPath();
  CHAR16 * PathPattern = L"ACPI\\*.aml";
  Print(L"%s\n", path);
  StrnCatS(path, StrLen(path) + StrLen(PathPattern) + 1, PathPattern, StrLen(PathPattern));
  Print(L"Opening ACPI Folder: %s\n", path);
  EFI_STATUS Status = gEfiShellProtocol->FindFiles(path, ACPIFolderHead);
  FreePool(path);
  return Status;
}

UINT64
ReadAMLFileToBuffer (
  const CHAR16 * AMLFile
  ) 
{
	SHELL_FILE_HANDLE FileHandle;
	UINT64 FileSize = 0;
  UINTN AMLBufferSize = 0;
  VOID * AMLBuffer = NULL;

  EFI_STATUS Status = gEfiShellProtocol->OpenFileByName(AMLFile, &FileHandle, EFI_FILE_MODE_READ);
  if (EFI_ERROR(Status)) {
      Print(L"ERROR: Could not open file\n");
      return 0;
  }

  gEfiShellProtocol->GetFileSize(FileHandle, &FileSize);

  AMLBufferSize = (UINTN)FileSize;
  AMLBuffer = AllocateZeroPool(AMLBufferSize);
  if (AMLBuffer == NULL) {
  	gEfiShellProtocol->CloseFile(FileHandle);
  	Print(L"Out of Resources\n");
  	return 0;
  }

  gEfiShellProtocol->ReadFile(FileHandle, &AMLBufferSize, AMLBuffer);
  if (EFI_ERROR (Status)) {
    gEfiShellProtocol->CloseFile(FileHandle);
    Print(L"Error reading file to buffer\n");
    return 0;
  }

  gEfiShellProtocol->CloseFile(FileHandle);
  return (UINT64)AMLBuffer;
}

EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *
FindFACP(
	EFI_ACPI_SDT_HEADER * Xsdt 
	)
{
  EFI_ACPI_SDT_HEADER *Entry;
  UINT32 EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
  UINT64 *EntryPtr = (UINT64 *)(Xsdt + 1);
  for (UINTN Index = 0; Index < EntryCount; Index++, EntryPtr++) {
    Entry = (EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr));
    if(Entry->Signature == EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) 
      return (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE *)Entry;
  }

  return NULL;
}

EFI_STATUS
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gBS = SystemTable->BootServices;
  gIH = ImageHandle;

  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER 	   *Rsdp 				      = NULL;
  EFI_ACPI_SDT_HEADER 							               *Xsdt 				      = NULL;
  EFI_SHELL_FILE_INFO 							               *ACPIFolderHead 	  = NULL;
  EFI_STATUS 										                   Status 				    = EFI_SUCCESS;
	UINT64 											                     XsdtEnd 			      = 0;

  EfiGetSystemConfigurationTable(&gEfiAcpi20TableGuid, (VOID **)&Rsdp);

  if (!Rsdp) {
      Print(L"ERROR: Could not find a RSDP.\n");
      return 1;
  }

  Xsdt = (EFI_ACPI_SDT_HEADER *)(Rsdp->XsdtAddress);
  Print(L"Found XSDT of size 0x%x (%u) at 0x%p\n", Xsdt->Length, Xsdt->Length, Xsdt);
  XsdtEnd =  Rsdp->XsdtAddress + Xsdt->Length;

  UINT32 EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
  Print(L"XSDT Entry Count: %u\n", EntryCount);

  gBS->LocateProtocol(&gEfiShellProtocolGuid, NULL, (VOID **)&gEfiShellProtocol);
  if(!gEfiShellProtocol) {
  	Print(L"Error: Could not find Shell Protocol\n");
  	return 1;
  }

  Status = OpenACPIFolder(&ACPIFolderHead);
  if(EFI_ERROR(Status)) {
  	Print(L"Error Opening Folder: %d\n", Status);
  	return 1;
  }

  EFI_SHELL_FILE_INFO * AMLFile = (EFI_SHELL_FILE_INFO *)ACPIFolderHead->Link.ForwardLink;

  UINT64 BufferLocation =  0;

  while (AMLFile->FullName != NULL) 
  { 
  	BufferLocation = ReadAMLFileToBuffer(AMLFile->FullName);
    if(!StrnCmp(AMLFile->FileName, L"DSDT.aml", 9)) {
      EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE * Facp = FindFACP(Xsdt);
      if(Facp){
        Print(L"Patching DSDT location from: 0x%x ", Facp->Dsdt);
        Facp->Dsdt = (UINT32)BufferLocation;
        Print(L"to: 0x%x\n", Facp->Dsdt);
      }
    } else if(StrnCmp(&AMLFile->FileName[0], L".", 1) &&
            StrnCmp(&AMLFile->FileName[0], L"_", 1)) {
      ((UINT64 *)XsdtEnd)[0] = BufferLocation;
      Xsdt->Length = Xsdt->Length + sizeof(UINT64);
      XsdtEnd =  Rsdp->XsdtAddress + Xsdt->Length;
    }

   	AMLFile = (EFI_SHELL_FILE_INFO *)AMLFile->Link.ForwardLink;
  } 

  EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
  Print(L"XSDT Patched\nNew Entry Count: %u\n", EntryCount);

  return EFI_SUCCESS;
}
