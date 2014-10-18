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

### Docker

Shaltúre also supports running inside Docker as a base for projects that build 
on top of it. Simply use `shalture/shalture` as your base image and then go 
from there. In the Docker image Shaltúre is installed to `/home/shalture/run/`

Example Dockerfile:

```Dockerfile
FROM shalture/shalture

ADD shalture.conf /home/shalture/run/etc/shalture.conf
```

runit will then start Shaltúre as normal, but with the configuration file you 
specified.

## Contact Us

 * [GitHub](http://www.github.com/Shaltúre/Shaltúre)
 * [IRC](irc://irc.freenode.net/#shalture)
