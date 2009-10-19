File System Recoverability test support

File specifications should placed in a text file named CorruptFileNames.lst
The specification should be in the format
pathName, returnCode, [ONCE|EVERY]
where
pathName is the fully qualified pathname of the file deemed corrupt
returnCode is an integer error code which is to be returned if an attempt is made to open pathName
ONCE means that only the first access to the file will return the specified error code
EVERY means that every access to the file will return the specified error code

The example file specification lines below are reproduced from \f32test\server\corruptTest\CorruptFileNames.lst 
z:\system\data\BadFile1.txt, -6, ONCE
z:\system\data\BadFile2.txt  ,    -20   ,every    

CorruptFileNames.lst will be searched for across all drives in the order used by TFindFile::FindByDir
using directories in the order given below
\
\sys\data\
\system\data

