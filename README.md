![Sevgi Engine](https://s14.gifyu.com/images/bsCeR.gif)

Sevgi Engine is a new open-source video game engine designed for classic Amiga computers. It delivers the necessary tools and generates the boilerplate code to enable the development of high-performance Amiga games using only the C programming language. It is built around the great [ScrollingTricks](https://aminet.net/package/dev/src/ScrollingTrick) by Georg Steger.

### Features
* **Performance**
  <br>All display algorithms aim to perform at a locked 50fps on a single buffered[^2] native Amiga chipset display. It implements an optimized version of the algorithm Scroller_XYUnlimited2_64 from [ScrollingTricks](https://aminet.net/package/dev/src/ScrollingTrick). Benefits highly from Fast Ram where available.</br>
* **System friendly**
  <br>Aims to work on all ROM versions (2.0+) providing a clean quit back to OS without the need for WHDLoad.</br>
* **AGA Support**
  <br>Supports all features provided by the AGA chipset.</br>
* **No limitations**
  <br>Designed to support all the well-known visual tricks used on popular Amiga video game titles while not limiting more experimental effects from being implemented.</br>
* **UI**
  <br>Code to create customizable mouse driven GUI objects (buttons, input gadgets, checkboxes etc.) with auto-layout and modern features, which can also be accessed by the keyboard or the joysticks.</br>
* **Controllers**
  <br>Supports very optimized input from mice, joysticks and CD32 JoyPads ***NEW*** from both joystick ports, as well as the keyboard.</br>
* **Easy development**
  <br>A native editor program is provided to generate code, manage game assets, edit color palettes etc. called **Sevgi Editor**. Other elements like game logic, animation and events require programming knowledge in C. The programming and compiling can be made natively (on the Amiga OS - using native compilers) or cross platfrom (on Windows or Linux PCs - using cross development tools). Sevgi Editor can import game maps made in [Tiled](https://www.mapeditor.org/).</br>
* **Templates**
  <br>Generates ready to compile and run game code from template genres which aims to ease bootstrapping. The templates include test assets.</br>
* **ptplayer**
  <br>Implements the great [ptplayer](https://aminet.net/package/mus/play/ptplayer) by Frank Wille for music and audio effects.</br>
* **No third party dependencies**
  <br>The game executable will not require any libraries[^2]</br>
* **Documentation**
  <br>The engine code is well-commented and comprehensively documented in AmigaGuide format.</br>

[^1]:Now also supports double buffering.
[^2]:Except diskfont.library. And even that is avoidable if you do not use any Amiga font asset.

### Sevgi Editor
Sevgi Engine code and template assets are generated using the provided editor called Sevgi Editor.
![Sevgi Editor](https://s14.gifyu.com/images/bsCei.png)

### Requirements
* ECS or AGA Amiga
* Kickstart 2.0+
* MUI 3.8 (for Sevgi Editor)

### Build
This source code which creates Sevgi Editor (which then can generate the engine code) is developed to be compiled with gcc (using any of [adtools](https://github.com/jens-maus/amigaos-cross-toolchain) or [bebbo's](https://github.com/bebbo/amiga-gcc) cross toolchains[^2]).
To compile for 68k[^3] target:

`make`

Compiled binaries can also be acquired from [aminet](https://aminet.net/package/dev/c/Sevgi_Engine).

Sevgi Engine code is suitable to be compiled with any Amiga compiler. Makefiles for SAS/C and gcc are provided and these compilers are tested to work. It includes and uses SDI_headers, so (hopefully) it is compiler agnostic.

[^2]:These toolchains should be built using MUI and CGX prefixes of course.
[^3]:Since this source includes and utilizes SDI_headers, it can be compiled also for MorphOS and AmigaOS4 targets, yet these two are not tested.

### Templates
Sevgi Editor comes with some ready to compile and run game templates from different genres which demonstrates its features using some free assets.

![TopDownTemplate.gif](https://s14.gifyu.com/images/bsUeC.gif)![PlatformerTemplate](https://s14.gifyu.com/images/bsCej.gif)

### Licences
Sevgi Engine and Sevgi Editor is under [MIT Licence](https://opensource.org/license/mit). Meaning you can use it in whatever project you want as long as you include the original copyright and license notice.
ptplayer is public domain under [The Unlicence](https://opensource.org/license/unlicense) licence.

### Future Plans
Sevgi Engine is still a work in progress. Some new features planned are:
* Support for different level display implementations (i.e double buffering, split screen etc.)
* More graphical effects
* A native tilemap and gameobject editor
* More templates for different game genres

### Contributions
If you make use of Sevgi Engine and find shortcomings, contributions like feature requests, bug reports or even pull requests will be highly appreciated. Please don't hesitate to fork this repository and open a pull request to add your features and/or bugfixes.
