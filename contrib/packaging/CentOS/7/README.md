# RPM packaging specfile for CentOS 7

## Overview
This is bootstrapping specfile. It is able to
determine git repo owner, branch, tag and commit, then
reuse this information during package build.

## Requirements
You should have **rpm-devel** package installed,
also all those packages which are required by ReOpenLDAP
itself.

## Usage
A couple of commands is required to build the package:

>spectool -R -g reopenldap.spec  
>rpmbuild -bb reopenldap.spec

First command downloads source file to the directory
where rpmbuild expects to find it. Second command builds
a set of binary packages.

##Tips and tricks
If you ever need to find out
package file paths, you could use this command:
>spectool -R -g reopenldap.spec  
>rpmbuild -bb reopenldap.spec 2>&1 | tee /tmp/build.log  
>grep -n -E '(Wrote: )(.+)$' /tmp/build.log | awk '{print $2;}'

## Authors
  Specfile initially has been contributed by Ivan Viktorov
  (https://github.com/Ivan-Viktorov) as a comment on issue #34
  of original project
  (https://github.com/ReOpen/ReOpenLDAP/issues/33#issuecomment-249861076).

  Tune-up and bootsrapping has been implemented by
  Sergey Pechenko (https://github.com/tnt4brain/ReOpenLDAP/tree/devel)
