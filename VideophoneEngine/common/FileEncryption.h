#pragma once
#include <cstdio>
#include <string>

class FileEncryption
{
public:
	FileEncryption (std::string fileName);
	bool encrypt ();
	std::string decrypt ();

private:
	bool encryptOrDecrypt (FILE* in, FILE* out, bool do_encrypt);
	std::string m_fileName;
};

