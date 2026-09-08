# skif-test

## Description

Morse decoder for practice use, with [SKIF](https://github.com/jg1uaa/skif-arduino)(simple key interface) and libinput.

## Usage

```
$ skif-test [options]
```

## Options

<dl>
 <dt><code> -l &lt;device&gt;</code>
 <dd>Device file, <code>/dev/ttyACM0</code> (SKIF) or <code>seat0</code> (libinput) is default.
 <dt><code> -k</code>
 <dd>Use libinput interface.
 <dt><code> -s</code>
 <dd>Use SKIF interface. [default]
 <dt><code> -j</code>
 <dd>Use kana (Japanese) decoder.
 <dt><code> -e</code>
 <dd>Use alphabet (English) decoder. [default]
 <dt><code> -v</code>
 <dd>Verbose mode. Display will be "element status / duration (msec)".
</dl>

## Note

libinput mode (-k) may not function as intended or fail to open devices, depending on your desktop environment (such as X11/Wayland active sessions) or a lack of root or input group privileges.


## License

GPL v3.0 or later