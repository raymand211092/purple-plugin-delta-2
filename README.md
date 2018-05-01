# Delta Chat plugin for libpurple

## Overview

[Delta Chat](https://delta.chat) is an instant messaging application based on
email - which is to say, SMTP+IMAP.

It doesn't have its own desktop application at present, but it does have a
[core protocol library](https://github.com/deltachat/deltachat-core). Wrapping
this in a libpurple plugin will allow existing desktop applications such as
[Pidgin](https://pidgin.im) to interoperate.

## Build

Very basic instructions at present. First, `deltachat-core` isn't packaged, so
you'll need to build and install it according to
[these instructions](https://github.com/deltachat/deltachat-core/blob/master/README.md#build).

Now, you'll need the libpurple build dependencies:

```
sudo apt install libpurple-dev build-essential
```

Finally, run `make` to create a `libdelta.so` file.

## Use

The easiest way to use this is to copy the `libdelta.so` file into
`~/.purple/plugins`. When running pidgin, you'll now have the option to add
a "Delta Chat" account.

At present, the "Username" and "Password" account fields correspond to email
address and password, respectively. Many important settings also show up on the
"Advanced" tab - if left blank, the plugin will attempt to automatically detect
the correct values, but you may need to fill some of them in manually to get
the connection to work.

Run pidgin with `--debug` to see interesting output.

## Limitations

NOTHING IS DONE YET.

Once that's fixed:

There's no facility at present to import account keys, so sharing an email
address between your mobile and desktop isn't amazing. It's high on the agenda.
