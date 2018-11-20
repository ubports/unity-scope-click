# Unity-scope-click development docs

It is recommended to use [crossbuilder](#crosscompile) for platform-independent building. You can also [build natively](#build-natively), but keep in mind that you will have to install lots of dependencies on your host OS.

## Crosscompile

To crosscompile, we have a tool called [crossbuilder](https://github.com/ubports/crossbuilder), this will automatically build and deploy to the device. [Here's how to set it up](https://docs.ubports.com/en/latest/systemdev/testing-locally.html#cross-building-with-crossbuilder).

To build deb package just run:
```
crossbuilder
```

or if you want to build without packaging:
```
crossbuilder --no-deb
```

Use crossbuilder shell if you want to define your own cmake options. (this will not automaticly deploy to your device!)
```
crossbuilder shell
cd unity-scope-click
mkdir build-arm
cmake .. {OPTION(S)}
make
```

For these changes to take effect, you will need to restart unity-scope-click, you can do this with `restart unity8-dash` or reboot the device.

# Build natively

## Install deps

You need to add repo.ubports.com to your sources.list. 

Here is an example for bionic:
```
sudo apt install gnupg
echo "deb http://repo.ubports.com/ bionic main" | sudo tee /etc/apt/sources.list.d/ubports.list 
wget http://repo.ubports.com/keyring.gpg -O - | sudo apt-key add
sudo apt update
```

Then run mk-build-tools to install all the dependencies needed to build unity-scope-click.
```
sudo mk-build-deps -i
```

## Build

Then make build directory `mkdir build && cd build`

Then run cmake `cmake ..` here you can also provide your build options

Now its the fun part, build! `make -j4` (-j defines number of threads to build on)

# Tests

To run tests,

```
make check
```

