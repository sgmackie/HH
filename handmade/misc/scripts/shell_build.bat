@echo off

pushd handmade\misc
call shell.bat
popd

pushd handmade\code
call build
popd

cd build