# Define the library name
set(VPE_LIB stiHttp)

# Define the sources that are added to the library
set(${VPE_LIB}_SOURCES
  CstiHTTPService.cpp
  CstiURLResolve.cpp
  CstiHTTPTask.cpp
  stiSSLTools.cpp
  stiHTTPServiceTools.cpp
  SslCertificateHandler.cpp
)

set(${VPE_LIB}_LIBRARIES
  ${Poco_LIBRARIES}
)

# Define the API header files for the library
set(${VPE_LIB}_API_INCLUDE_FILES
  CstiHTTPService.h
  stiHTTPServiceTools.h
)

# Include common cmake file for VidephoneEngine
include(${VPE_COMMON_CMAKE})

install(FILES CAfile.crt DESTINATION share/lumina/certs)
install(FILES amazon-combined.pem DESTINATION share/lumina/certs)
install(FILES gdroot-g2.crt DESTINATION share/lumina/certs)
install(FILES gdig2.crt.pem DESTINATION share/lumina/certs)
install(FILES TrustedRoot-DigiCertCA.pem DESTINATION share/lumina/certs)
