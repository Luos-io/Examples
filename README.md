<a href="https://luos.io"><img src="https://uploads-ssl.webflow.com/601a78a2b5d030260a40b7ad/602f8d74abdf72db7f5e3ed9_Luos_Logo_animation_Black.gif" alt="Luos logo" title="Luos" align="right" height="60" /></a>

[![](http://certified.luos.io)](https://luos.io)
[![](https://img.shields.io/github/license/Luos-io/Examples)](
https://github.com/Luos-io/Examples/blob/master/LICENSE)
[![](https://img.shields.io/reddit/subreddit-subscribers/Luos?style=social)](https://www.reddit.com/r/Luos)


# Luos examples

This repository is a submodule of the [Luos library](https://github.com/Luos-io/Luos) repository. It contains examples of Containers and Apps for using Luos. This repository can be used on it's own, or as a submodule for the Luos Library directly.

Feel free to use these examples as a starting point for your own projects, and don't forget to share your creations on the [Luos forum](https://community.luos.io/)!

## Content

### Apps folder

This folder contains the container's sources of each **App** project (see [Apps section](https://docs.luos.io/pages/low/containers/create-containers.html?#apps-guidelines) in documentation). An App is the intelligence of your project, and the examples provided will show you how to set up an App for your project.

### Projects folder

This folder contains the sources of every examples.  It is sorted into two folders; Arduino for SAM-D based Arduino boards, and L0, for the Luos evaluation and prototyping boards. Inside these folders are a collection of different **drivers** and **projects** for you to use as they are, or to use as a starting point for your own projects.

#### Prototyping boards projects

Most of the projects are Luos prototyping boards (see [Demonstration boards section](https://docs.luos.io/pages/demo_boards/demo-boards.html) in documentation).

#### Hardware folder

This contains hardwares librairies used by Luos to inspire you to create your own designs:
 - **l0**: The base board for using Luos [demonstration boards](https://docs.luos.io/pages/demo_boards/demo-boards.html).
 - **l0-shields**: Different shields that can be used directly on the Luos L0 board for added functionality.
 - **stm32f0_disco_luos_shield**: a debugging shield equivalent to a Luos L0 for STM32F072B-DISCO dev board.
 - **Breakout_board**: a small add-on board to make any board compatible with Luos.
 - **wiring_and_power**: a set of projects for Luos wires and power input boards.
 - **00_Common_Libraries**: a set of Kicad libraries commonly used.

##### How to start

1. install KiCad: https://www.kicad-pcb.org/

2. Once Kicad is installed, open it and setup libraries. There are 3 libs:

	- **Schematic**: You must go on "Preferences" > "Manage Symbol Librairies". Click on the (+) button and type: Common_Lib in Nickname field  and type the path of the Common_Lib.lib file in Library Path field (Electronics/00_Common_Libraries/Common_Library/Common_Lib.lib).

	- **PCB**: You must go on "Preferences" > "Manage Footprint Librairies". Click on the (+) button and type: Common_Footprint in Nickname field  and type the path of the Common_Footprint.pretty file in Library Path field(Electronics/00_Common_Libraries/Common_Footprint.pretty).

	- **3D body**: You must go on "Preferences" > "Configure Paths". On the KISYS3DMOD line type the path of the Common_Mecanic Folder(Electronics\00_Common_Libraries\Common_Mecanic).


3. Click on File > Open project

4. Go to the KiCad project folder you want open and click on the .pro file.

## Don't hesitate to read [our documentation](https://docs.luos.io), or to post your questions/issues on the [Luos' subreddit](https://www.reddit.com/r/Luos/). :books:

[![](https://img.shields.io/reddit/subreddit-subscribers/Luos?style=social)](https://www.reddit.com/r/Luos)
[![](https://img.shields.io/badge/Luos-Documentation-34A3B4)](https://docs.luos.io)
[![](https://img.shields.io/badge/LinkedIn-Follow%20us-0077B5?style=flat&logo=linkedin)](https://www.linkedin.com/company/luos)
