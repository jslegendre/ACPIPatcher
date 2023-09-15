# ACPIPatcher
An EFI application and driver to add SSDTs and/or patch in your own DSDT

I made this tool because I wanted a way to use [RehabMans ACPI Debug tool](https://github.com/RehabMan/OS-X-ACPI-Debug) on my MacBook Pro without using Clover.  Although I made this with macOS in mind it will work with any OS along with any bootloader (provided your bootloader does not do its own ACPI patching).

## How to use:
Using this is fairly straighforward. Just place a folder titled `ACPI` in the same directory as `ACPIPatcher.efi` and it will search the folder for any `.aml` files.  Any file not named `DSDT.aml` will be added to the XSDT table as SSDTs while `DSDT.aml` will completely replace the OEM DSDT. Once you have everything in place just call `ACPIPatcher.efi` from an EFI shell and reboot when you are done.

However, if you would like your patches to survive reboots you can use the driver version of this software.  To do this simply place  `ACPIPatcherDxe.efi` in your bootloaders driver folder along with the `ACPI` folder mentioned above. 

I have also provided a test SSDT in `Build/ACPI`. 

Your directory listing should look something like this:
```
FS0:> ls
<DIR> ACPI
      ACPIPatcher.efi

FS0:> ls ACPI/
      SSDT-TEST.aml
      DSDT.aml
```

## How to Build:
Place `ACPIPatcherPkg` in your EDKII folder and build as usual. Prebuilt binaries are provided however. 
`build -p ACPIPatcherPkg/ACPIPatcherPkg.dsc`
On macOS with Xcode build as follows:
```
brew install nasm
brew install mtoc
git clone --depth=1 -b edk2-stable202202  https://github.com/tianocore/edk2.git && cd edk2/
git submodule update --init --recommend-shallow
. ./edksetup.sh
make -C BaseTools
build -a X64 -b RELEASE -t XCODE5 -p ACPIPatcherPkg/ACPIPatcherPkg.dsc
build -a X64 -b DEBUG -t XCODE5 -p ACPIPatcherPkg/ACPIPatcherPkg.dsc
```


### To Do:
This is my first EFI application so I'm sure there is much work to be done here.  Things I can think of:

* ~~Rewrite as a driver so the application does not need to be called before every boot~~
* Better memory management/error handling??
