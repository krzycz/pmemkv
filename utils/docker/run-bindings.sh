#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# run-bindings.sh - checks bindings' building and installation
#

set -e

source `dirname ${0}`/prepare-for-build.sh

# master: Merge pull request #44 from lukaszstolarczuk/update-tr..., 21.11.2019
RUBY_VERSION="3741e3df698245fc8a15822a1aa85b5c211fd332"

# common: 1.2.0 release, 02.07.2021
JAVA_VERSION="9a32f9f518198ae575242b448f61514c231b5a60"

# master: Merge pull request #55 from lukaszstolarczuk/fix-comment, 03.04.2020
NODEJS_VERSION="76600e002b9d9105d3f46b7cc2bf991931286cec"

# master: ver. 1.0, 03.03.2020
PYTHON_VERSION="094bc84fdabff81c2eb2017d32caad2582835f90"

# build and install pmemkv
cd ${WORKDIR}
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=${PREFIX} \
	-DBUILD_DOC=OFF \
	-DBUILD_EXAMPLES=OFF \
	-DBUILD_TESTS=OFF
make -j$(nproc)
sudo_password -S make -j$(nproc) install

echo
echo "##################################################################"
echo "### Verifying building and installing of the pmemkv-ruby bindings "
echo "##################################################################"
cd ${WORKDIR}
git clone https://github.com/pmem/pmemkv-ruby.git
cd pmemkv-ruby
git checkout ${RUBY_VERSION}
mkdir -p vendor/cache/
cp -r /opt/bindings/ruby/* vendor/cache/
bundle install --local
bundle exec rspec

cd ..
rm -rf pmemkv-ruby

echo
echo "##################################################################"
echo "### Verifying building and installing of the pmemkv-java bindings "
echo "##################################################################"
cd ${WORKDIR}
git clone https://github.com/pmem/pmemkv-java.git
cd pmemkv-java
git checkout ${JAVA_VERSION}
mkdir -p ~/.m2/repository
cp -r /opt/bindings/java/repository ~/.m2/
mvn install -e

cd ..
rm -rf pmemkv-java

echo
echo "####################################################################"
echo "### Verifying building and installing of the pmemkv-nodejs bindings "
echo "####################################################################"
cd ${WORKDIR}
git clone https://github.com/pmem/pmemkv-nodejs.git
cd pmemkv-nodejs
git checkout ${NODEJS_VERSION}
cp -r /opt/bindings/nodejs/node_modules .
npm install --save
npm test

cd ..
rm -rf pmemkv-nodejs

echo
echo "####################################################################"
echo "### Verifying building and installing of the pmemkv-python bindings "
echo "####################################################################"
cd ${WORKDIR}
git clone https://github.com/pmem/pmemkv-python.git
cd pmemkv-python
git checkout ${PYTHON_VERSION}
python3 setup.py bdist_wheel
pip3 install dist/pmemkv-*.whl --user

cd tests
python3 -X faulthandler -m pytest -v pmemkv_tests.py
python3 -X faulthandler -m pytest -v nontrivial_data_tests.py

cd ../examples
PMEM_IS_PMEM_FORCE=1 python3 basic_example.py

cd ../..
rm -rf pmemkv-python
