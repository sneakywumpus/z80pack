For installation on OSX you need compiler command line tools, xcode is
not mandatory. If you execute gcc --version on a new system, it will
tell you the command to install compilers.

You need to install the SDL2 and SDL2_image frameworks from
https://github.com/libsdl-org/SDL/releases/
and
https://github.com/libsdl-org/SDL_image/releases/

Grab the latest SDL2-2.x.x.dmg and SDL2_image-2.x.x.dmg, and copy
the framework bundles into /Library/Frameworks (create this directory
if it doesn't exist).

Now you are all set and should be able to build the z80pack machines
from source, following the other instructions here.
