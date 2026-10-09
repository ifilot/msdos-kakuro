# Borland toolchain provenance

This directory includes the DOS components of Borland C++ & Application
Frameworks 3.1, dated June 10, 1992. Original copyright notices in the binaries,
headers, libraries, and documentation are retained. These third-party files
remain proprietary and are separate from the project's own code.

Source: [WinWorld's Borland C++ 3.x collection](https://winworldpc.com/product/borland-c/30).
The exact release is
[Borland CPP 3.1 and Application Frameworks (1992) (3.5-1.44mb)](https://winworldpc.com/download/f7396558-9ba2-11e9-ab10-fa163e9022f0),
the English fifteen-disk set. Retrieved October 9, 2026.

Archive filename:
`Borland CPP 3.1 and Application Frameworks (1992) (3.5-1.44mb).7z`

Archive SHA-1, matching WinWorld's published checksum:
`c130504ebdba28d41ba95fc70a46e99a787d6efa`

Archive SHA-256:
`76e155a7fb7eb0590930ecd513644c977497a020466a272943776fbf683d0f27`

`import-borland.py` documents and reproduces extraction from the original media.
It extracts files using mtools and Info-ZIP, rejoins Borland's split ZIP volumes
after removing each five-byte volume header, and preserves the original files.
It adds `INCLUDE/SYS` copies of `STAT.H`, `TIMEB.H`, and `TYPES.H`, and generates
`BIN/BCC.CFG` and `BIN/TLINK.CFG` for the fixed `C:\BC` mount.

Included components: DOS command-line compiler/linker and utilities, DOS IDE
and help, Turbo Assembler, Turbo Debugger and utilities, C/C++ headers, runtime
and class libraries for the DOS memory models, and original documentation.
Windows IDE, OWL, examples, profiler, and BGI drivers are omitted.

`SHA256SUMS` records every installed file. Installation media and temporary
host tools are not part of this repository.
