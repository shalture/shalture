## Shaltúre

Shaltúre is a set of services for IRC networks designed for large IRC networks 
with high scalability requirements.  It is relatively mature software, with 
some code and design derived from another package called Shrike, and is a fork 
of a dying services package named Atheme.

Shaltúre's behavior is tunable using modules and a highly detailed 
configuration file. Almost all behavior can be changed at deployment time just 
by editing the configuration.

If you are running this code from Git, you should read `GIT-Access` for 
instructions on how to fully check out the Shaltúre tree, as it is spread 
across a few repositories.

## Building

Whatever you do, make sure you do *not* install Shaltúre into the same location 
as the source. Shaltúre will default to installing in `$HOME/shalture`, so make 
sure you plan accordingly for this.

    $ git submodule update --init
    $ ./configure
    $ make
    $ make install

If you're still lost, read [INSTALL](INSTALL) or [GIT-Access](GIT-Access) for 
hints.

## Contact Us

__Be sure to read [the FAQ](doc/FAQ.md) as well.__

 * [GitHub: shalture/shalture](https://github.com/shalture/shalture)
 * [IRC: chat.freenode.net, #shalture](ircs://chat.freenode.net:6697/shalture) ([webchat](https://kiwiirc.com/client/chat.freenode.net:+6697/shalture))
