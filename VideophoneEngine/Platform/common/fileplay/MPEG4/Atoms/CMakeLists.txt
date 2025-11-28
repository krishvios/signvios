# Define the library name
set(VPE_LIB stiPlatform-common-fileplay-MPEG4-Atoms)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  Atom.cpp
  FullAtom.cpp
  UdtaAtom.cpp
  TrakAtom.cpp
  TkhdAtom.cpp
  MvhdAtom.cpp
  MoovAtom.cpp
  FtypAtom.cpp
  MdiaAtom.cpp
  MinfAtom.cpp
  StblAtom.cpp
  MdhdAtom.cpp
  HdlrAtom.cpp
  VmhdAtom.cpp
  SmhdAtom.cpp
  HmhdAtom.cpp
  NmhdAtom.cpp
  DrefAtom.cpp
  SttsAtom.cpp
  CttsAtom.cpp
  StsdAtom.cpp
  StszAtom.cpp
  StscAtom.cpp
  StcoAtom.cpp
  StssAtom.cpp
  ElstAtom.cpp
  TrefAtom.cpp
  UrlAtom.cpp
  UrnAtom.cpp
  DinfAtom.cpp
  UnknownAtom.cpp
)

set(${VPE_LIB}_PRIVATE_INCLUDE_DIRECTORIES
  ..
  ../Toolbox
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})
