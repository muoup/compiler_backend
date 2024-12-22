<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#notes">Usage</a></li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

<!-- ABOUT THE PROJECT -->
## About The Project

A simple compiler passion project to learn more about compilers and intermediate representation.
Developed loosely based on LLVM's IR -- none of the code here is based on the source code of LLVM,
however most high-level functionality is based on observing the compiled output of different source
code snippets using the Godbolt Compiler Explorer.

Currently, the project does not support a proper codegen API, however simple test cases can be run.
The ultimate goal of this project is to be compatible with my [compiler frontend project](https://github.com/muoup/compiler_frontend),
allowing for a full compiler toolchain from source code to x86-64 assembly.

Compilation from x86-64 assembly to machine code is currently performed using NASM and GCC. I would like
to research lessening the GCC dependency to just ld, however this is not currently a priority.

<!-- GETTING STARTED -->
## Getting Started

### Prerequisites

This software has been tested using GCC and CMake on Debian using WSL. It is likely that
this software is also compatible with Clang, however this has not been tested.

* GCC
  ```sh
  sudo apt-get install gcc
  ```
  
* CMake
  ```sh
  sudo apt-get install cmake
  ```

### Installation

The software only serves currently as a testable backend, and potentially a library for other projects
in the near future. As such the only installation step is to clone the repo, assuming the prerequisites
are installed.

1. Clone the repo
   ```sh
   git clone https://github.com/muoup/compiler_backend.git
   ```

<!-- NOTES -->
## Functionality and Notes

For notes on the passes performed by the compiler, see [Analysis Passes](/docs/analysis_passes.md).

For notes on the nodes used in the IR, see [IR Nodes](/docs/ir_nodes.md).

For notes on potential optimizations to be explored in the future, see [Optimizations](/docs/optimizations.md).

For example compilations, see [Example Compilations](/docs/example-compilations.md).

<!-- USAGE EXAMPLES -->
## Usage

The software is currently in a very early stage of development, and as such does not have a proper API. 
However, for examples of compiled output, see [Example Compilations](/docs/example-compilations).

<!-- ROADMAP -->
## Roadmap

- [ ] Test Suite w/ 100% instruction coverage
- [ ] Codegen API
- [ ] [Compiler Frontend](https://github.com/muoup/compiler_frontend) compatibility

<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

* [The LLVM Compiler Infrastructure Project](https://llvm.org/)
* [Godbolt Compiler Explorer](https://godbolt.org/)