<a href="https://luos.io"><img src="https://www.luos.io/wp-content/uploads/2020/03/Luos-color.png" alt="Luos logo" title="Luos" align="right" height="60" /></a>

[![](http://certified.luos.io)](https://luos.io)
[![](https://img.shields.io/github/license/Luos-io/Examples)](
https://github.com/Luos-io/Examples/blob/master/LICENSE)


# Luos examples

This repository is a submodule of the Luos repository. It contains examples and libraries for using Luos. 

Feel free to produce and use all our design to create your own projects.

## Content

### Apps folder

This folder contains the container's sources of each **app** project (see [Apps section](https://docs.luos.io/pages/low/containers/create-containers.html?#apps-guidelines) in documentation).

### Drivers folder

This folder contains the container's sources of each **driver** project (see [Drivers section](https://docs.luos.io/pages/low/containers/create-containers.html?#drivers-guidelines) in documentation).

### Projects folder

This folder contains the sources of every examples. 

#### Prototyping boards projects

Most of the projects are Luos prototyping boards (see [Protyping boards section](https://docs.luos.io/pages/prototyping_boards/proto-boards.html) in documentation).

#### Electronics basis

The folder `0_electronics_basis` contains hardwares librairies used by Luos to inspire you to create your own designs:
 - **L0**: The base board for using Luos [prototyping boards](https://docs.luos.io/pages/prototyping_boards/proto-boards.html).
 - **STm32f0_disco_luos_shield**: a debugging shield equivalent to an L0 for STM32F072B-DISCO dev board.
 - **Breakout_board**: a small add-on board to make any board compatible with Luos.
 - **wiring_and_power**: a set of projects for Luos wires and power input boards.
 - **00_Common_Libraries**: a set of Kicad libraries commonly used.

## How to start

1. install KiCad: https://www.kicad-pcb.org/

2. Once Kicad is installed, open it and setup libraries. There are 3 libs:

	- **Schematic**: You must go on "Preferences" > "Manage Symbol Librairies". Click on the (+) button and type: Common_Lib in Nickname field  and type the path of the Common_Lib.lib file in Library Path field (Electronics/00_Common_Libraries/Common_Library/Common_Lib.lib).

	- **PCB**: You must go on "Preferences" > "Manage Footprint Librairies". Click on the (+) button and type: Common_Footprint in Nickname field  and type the path of the Common_Footprint.pretty file in Library Path field(Electronics/00_Common_Libraries/Common_Footprint.pretty).

	- **3D body**: You must go on "Preferences" > "Configure Paths". On the KISYS3DMOD line type the path of the Common_Mecanic Folder(Electronics\00_Common_Libraries\Common_Mecanic).


3. Click on File > Open project

4. Go to the KiCad project folder you want open and click on the .pro file.

[![](https://img.shields.io/discourse/topics?server=https%3A%2F%2Fcommunity.luos.io&logo=Discourse)](https://community.luos.io)
[![](https://img.shields.io/badge/Luos-Documentation-34A3B4)](https://docs.luos.io)
[![](https://img.shields.io/badge/LinkedIn-Follow%20us-0077B5?style=flat&logo=linkedin)](https://www.linkedin.com/company/luos)
