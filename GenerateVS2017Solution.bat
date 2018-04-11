@echo off
setlocal enabledelayedexpansion
cd %~dp0

set target_dir=_build

if exist "%target_dir%" (
	rmdir /s /q %target_dir%
)

mkdir %target_dir%
cd %target_dir%
cmake .. -G	"Visual Studio 15 2017"
