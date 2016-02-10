@echo off
rem Simple .bat script for creating the NMake Makefile snippets.

if not "%1" == "header" if not "%1" == "file" if not "%1" == "footer" goto :error_cmd
if "%2" == "" goto error_no_destfile

if "%1" == "header" goto :header
if "%1" == "file" goto :addfile
if "%1" == "footer" goto :footer

:header
if "%3" == "" goto error_var
echo %3 =	\>>%2
goto done

:addfile
if "%3" == "" goto error_file
echo.	%3	\>>%2
goto done

:footer
echo.	$(NULL)>>%2
echo.>>%2
goto done

:error_cmd
echo Specified command '%1' was invalid.  Valid commands are: header file footer.
goto done

:error_no_destfile
echo Destination NMake snippet file must be specified
goto done

:error_var
echo A name must be specified for using '%1'.
goto done

:error_file
echo A file must be specified for using '%1'.
goto done

:done