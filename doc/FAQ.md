Frequently asked questions
--------------------------

1. Will you implement SQL support?

   Probably not.

   There are various impracticalities with SQL support, such as:

     - the inability to properly validate data being inserted back
       into the database
     - cache invalidation (caching would be needed to ensure
       performance)

   Additionally, implementing SQL support would require reworking
   a lot of code; while we're not fundamentally opposed to that,
   there is currently no compelling reason to support SQL.

   If you're looking for approaches for web integration, take a look
   at the XMLRPC and JSONRPC interfaces.

2. Which crypto module should I use?

   `crypto/posix`, unless you are importing from anope, in which case
   use the module that is appropriate for you.

3. There is a Shaltúre "0day exploit"?!&%&%@!!

   Report it to us.  Do not twitter about it, that is generally not
   helpful.  People who publicize 0days before they can be fixed are
   also generally known as "assholes."

   If the exploit applies to Atheme as well, you'll want to report it
   upstream to ensure all derived services packages can be fixed
   as soon as possible.

4. I get a compile error about some `.po` file thing.

   `./configure --disable-nls` or install gettext and rerun `./configure`.

5. Shaltúre exits complaining that it is 'tainted'.

   You have configured your network in a way that is not supported
   correctly by Shaltúre at this time.  In order to use this configuration,
   you must acknowledge that you are using an unsupported configuration.

   To do this, you enable the `allow_taint` directive in the config, but
   do note that if you come asking us for help with your installation,
   we will not provide you with support.

   Also note that we do not provide support for enabling `allow_taint`.

6. There hasn't been a stable release of Shaltúre yet. Can I use it on my network?

   It depends, really.

   Shaltúre is based on Atheme, which is in use on various major and minor
   production IRC networks. As such, some people are fine with using the
   latest code from git at their own risk.

   While we try to avoid breaking the `master` branch, it's happened before
   and probably will happen again. Thus we recommend setting up a test
   environment before you rely on the latest development code for
   production use.

   If you need a stable release that has been used on production networks
   for a while, we recommend using the latest Atheme release for now
   until we make a stable release of our own.
