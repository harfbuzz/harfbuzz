64-bit wine is under somewhat active development so a recent installation is
recommended. Fedora keeps it up-to-date and ships version 3.15 of both
32-bit and 64-bit as of the time of writing. This also allow `WOW64` i.e.
runnning both 32-bit and 64-bit window binaries in the same setting.

For the development of HarfBuzz, the Microsoft shaping technology, Uniscribe,
as a widely used and tested shaper is used as more-or-less OpenType reference
implemenetation and that specially is important where OpenType specification
is or wasn't that clear. For having access to Uniscribe on Linux/macOS these
steps are recommended:

1. Install Wine and mingw32/mingw64 from your package manager. This likely
   means, at the very least, running this:

```
sudo dnf install -y wine.i686 wine.x86_64 mingw32-gcc-c++ mingw64-gcc-c++
```

2. set up autoconf with `NOCONFIGURE=1 ./autogen.sh`

3. Build the win32 version of harbuzz with:

```
mkdir win32build
cd win32build && ../mingw32.sh --with-uniscribe && cd ..
make -Cwin32build
```

4. Build the win64 version of harbuzz with:

```
mkdir win64build
cd win64build && ../mingw64.sh --with-uniscribe && cd ..
make -Cwin64build
```

5. set up a new and clean `WINEPREFIX`, where wine puts all its user-specific files in. It is
   auto-populated if empty, when running any wine commands. For example, you can try
   running the command shell `cmd`, and just type `exit` when wine finishes launching:

```
mkdir /tmp/wine-testing
WINEPREFIX=/tmp/wine-testing wine64 cmd
```
Just type `exit` when the cmd `Z:\...someplace...\harfbuzz>` prompt appears.

6. Now you can use hb-shape using `WINEPREFIX=/tmp/wine-testing wine win32build/util/hb-shape.exe`
   or `WINEPREFIX=/tmp/wine-testing wine win64build/util/hb-shape.exe`, but if you like to
   to use the original Uniscribe,

7. Bring the Microsoft-built versions of `usp10.dll` from your Windows installation.
   On not-too-old 64-bit MS Windows, these are at `C:\Windows\SysWOW64\usp10.dll`
   and `C:\Windows\System32\usp10.dll`.

   You want to avoid the very latest ones, whch are DirectWrite proxies
   ([for more info](https://en.wikipedia.org/wiki/Uniscribe)).
   Rule of thumb, your `usp10.dll` should have a size more than 300kb (useable and interesting versions are 500k to 800k).
   Those designed to work with DirectWrite are about 80k and does not work with wine at the moment. Avoid those.

   Overwrite the wine-builtin ones in your `WINEPREFIX` with them. Before overwriting, these are about 4k each and shows:

```
$ file /tmp/wine-testing/drive_c/*/*/usp10.dll
/tmp/wine-testing/drive_c/windows/system32/usp10.dll: , created: Thu Jan  1 00:01:36 1970
/tmp/wine-testing/drive_c/windows/syswow64/usp10.dll: , created: Thu Jan  1 00:01:36 1970
```

   After overwriting, they should say:

```
$ file /tmp/wine-testing/drive_c/*/*/usp10.dll
/tmp/wine-testing/drive_c/windows/system32/usp10.dll: PE32+ executable (DLL) (GUI) x86-64, for MS Windows
/tmp/wine-testing/drive_c/windows/syswow64/usp10.dll: PE32 executable (DLL) (GUI) Intel 80386, for MS Windows
```

8. Now you can use Uniscribe with either

```
WINEPREFIX=/tmp/wine-testing WINEDLLOVERRIDES="usp10=n" wine win32build/util/hb-shape.exe fontname.ttf -u 0061,0062,0063 --shaper=uniscribe
```

or

```
WINEPREFIX=/tmp/wine-testing WINEDLLOVERRIDES="usp10=n" wine win64build/util/hb-shape.exe fontname.ttf -u 0061,0062,0063 --shaper=uniscribe
```

(`0061,0062,0063` means `abc`, use test/shaping/hb-unicode-decode to generate ones you need)

If you are mistakenly using DirectWrite proxies, you will likely see something similar to below:

```
000d:err:module:find_forwarded_export function not found for forward 'GDI32.ScriptBreak' used by L"C:\\windows\\system32\\usp10.dll". If you are using builtin L"usp10.dll", try using the native one instead.
000d:err:module:find_forwarded_export function not found for forward 'GDI32.ScriptStringAnalyse' used by L"C:\\windows\\system32\\usp10.dll". If you are using builtin L"usp10.dll", try using the native one instead.
000d:err:module:find_forwarded_export function not found for forward 'GDI32.ScriptStringCPtoX' used by L"C:\\windows\\system32\\usp10.dll". If you are using builtin L"usp10.dll", try using the native one instead.
000d:err:module:find_forwarded_export function not found for forward 'GDI32.ScriptStringFree' used by L"C:\\windows\\system32\\usp10.dll". If you are using builtin L"usp10.dll", try using the native one instead.
...
```
