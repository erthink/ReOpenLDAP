#!/usr/bin/perl -w -T
# $ReOpenLDAP$
## Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
## Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
##
## This file is part of ReOpenLDAP.
##
## ReOpenLDAP is free software; you can redistribute it and/or modify it under
## the terms of the GNU Affero General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## ReOpenLDAP is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Affero General Public License for more details.
##
## You should have received a copy of the GNU Affero General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##
## ---
##
## Copyright 2007-2014 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
##
## ACKNOWLEDGEMENTS:
## This work was initially developed by Brian Candler for inclusion
## in OpenLDAP Software.

# See: http://search.cpan.org/dist/Net-Server/

package ExampleDB;

use strict;
use vars qw(@ISA);
use Net::Server::PreFork; # any personality will do

@ISA = qw(Net::Server::PreFork);

ExampleDB->run(
  port=>"/tmp/example.sock|unix"
  #conf_file=>"/etc/example.conf"
);
exit;

### over-ridden subs below
# The protocol is the same as back-shell

sub process_request {
  my $self = shift;

  eval {

    local $SIG{ALRM} = sub { die "Timed Out!\n" };
    my $timeout = 30; # give the user 30 seconds to type a line
    alarm($timeout);

    my $request = <STDIN>;

    if ($request eq "SEARCH\n") {
      my %req = ();
      while (my $line = <STDIN>) {
        chomp($line);
        last if $line eq "";
        if ($line =~ /^([^:]+):\s*(.*)$/) { # FIXME: handle base64 encoded
          $req{$1} = $2;
        }
      }
      #sleep(2);  # to test concurrency
      print "dn: cn=test, dc=example, dc=com\n";
      print "cn: test\n";
      print "objectclass: cnobject\n";
      print "\n";
      print "RESULT\n";
      print "code: 0\n";
      print "info: answered by process $$\n";
    }
    else {
      print "RESULT\n";
      print "code: 53\n";  # unwillingToPerform
      print "info: I don't implement $request";
    }

  };

  return unless $@;
  if( $@=~/timed out/i ){
    print "RESULT\n";
    print "code: 3\n"; # timeLimitExceeded
    print "info: Timed out\n";
  }
  else {
    print "RESULT\n";
    print "code: 1\n"; # operationsError
    print "info: $@\n"; # FIXME: remove CR/LF
  }

}

1;
