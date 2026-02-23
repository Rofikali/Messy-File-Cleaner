

### inatallation packeges ( on MINGW TERMINAL )
    pacman -S mingw-w64-x86_64-libjpeg-turbo
    pacman -S mingw-w64-x86_64-make
    pacman -S mingw-w64-x86_64-cppcheck

    pacman -S libjpeg-turbo
    pacman -S make

### REMOVE PACKEGES
    pacman -R mingw-w64-x86_64-make
    
### filter installed packeges 
    pacman -Q
    pacman -Q | grep make
    pacman -Q | grep jpeg
    mingw-w64-x86_64-make 4.4.1-4
    mingw-w64-x86_64-libjpeg-turbo 3.1.3-1


### Find DLL Inside FILES
    nm -g build/img2pdf.dll | grep img2pdf_

### RUN ON CMDTerminal with Makefile
    mingw32-make clean
    mingw32-make
    where mingw32-make 
    cleaner.exe --source
    cleaner.exe --source "C:\Users\Flex 1\Desktop\Rofik don\Softwares\C Programming\cleaner\Testing" --target "C:\Users\Flex 1\Desktop\Rofik don\Softwares\C Programming\cleaner\Testing" --mode creation

    cleaner.exe --source "C:\Users\Flex 1\Desktop\Rofik don\Softwares\C Programming\cleaner\Testing" --target "C:\Users\Flex 1\Desktop\Rofik don\Softwares\C Programming\cleaner\Testing" --mode modified
    

### RUN ON MINGWTerminal
    make clean
    make
    where make
    make clean
    make run     
    ./build/cleaner.exe "D:\Testing" --recursive --dry-run


### PGO
    mingw32-make MODE=pgo-generate
    # run your program to generate profile data
    mingw32-make clean
    mingw32-make MODE=pgo-use