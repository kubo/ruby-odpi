# Ruby-ODPI

Ruby-ODPI is a ruby interface for Oracle Database based on [ODPI-C][].

Ruby-ODPI consists of the following two layers.

* High-level API - easy-to-use API based on the following layer.
* Low-level API - straightforward binding of ODPI-C functions.

## Status

* High-level API - Nothing
* Low-level API - In development

## Documents

http://www.rubydoc.info/github/kubo/ruby-odpi/master

## License

Ruby-ODPI itself is under [2-clause BSD-style license](https://opensource.org/licenses/BSD-2-Clause).

The license of documentation comments about the low-level API is same
with ODPI-C because most of them were copied from ODPI-C documents.

ODPI-C bundled in Ruby-ODPI is under the terms of:

1. [the Universal Permissive License v 1.0 or at your option, any later version](http://oss.oracle.com/licenses/upl); and/or
2. [the Apache License v 2.0](http://www.apache.org/licenses/LICENSE-2.0). 

[ODPI-C]: https://github.com/oracle/odpi/
