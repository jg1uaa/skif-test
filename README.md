# skif-test

## Description

Morse decoder for practice use, with [SKIF](https://github.com/jg1uaa/skif-arduino) (simple key interface) and libinput.

## Usage

```
$ skif-test [options]
```

## Options

<dl>
 <dt><code> -l &lt;device&gt;</code>
 <dd>Device file, <code>/dev/ttyACM0</code> (SKIF) or <code>seat0</code> (libinput) is default.
 <dt><code> -d &lt;msec&gt;</code>
 <dd>Basetime (dot time) for judge elemnts. Default 100 (msec).
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

## Element status

|character|status|time range (basetime x N)|
|--|--|--|
|X|Too short dit|0 < N < 0.5|
|.|Dit|0.5 <= N < 1.5 |
|?|Could not determine dit or dah|1.5 <= N < 2|
|-|Dah|2 <= N < 6|
|=|Too long dah|6 <= N|
|x|Too short element space|0 < N < 0.5|
|_|Element space (verbose mode only)|0.5 <= N < 1.5|
|!|Too long element space|1.5 <= N < 2|
|~|Character space (verbose mode only)|2 <= N < 4|
|#|Word space (verbose mode only)|4 <= N|

## Example

### with SKIF

```
$ ./skif-test -l /dev/ttyU0
wait for device...
device ready
-.-. [C] --.- [Q] -.-. [C] --.- [Q]
-.. [D] . [E]
X--- [*]
--. [G] .---- [1] ..- [U] X- [*] .- [A]
-.- [K]
^C
$
```

### with libinput, verbose mode and Japanese decorder

```
$ ./skif-test -k -v -j
press [Esc] to quit
. 110.001
_ 79.996
- 250.001
# 1000.000
* .- [イ]

. 90.001
_ 89.998
- 219.999
_ 100.001
. 80.000
_ 59.999
- 320.001
# 1000.000
* .-.- [ロ]

- 260.003
_ 79.998
. 79.999
_ 80.003
. 69.998
_ 79.930
. 70.073
# 1000.000
* -... [ハ]

^C
$
```

## Note

libinput mode (-k) may not function as intended or fail to open devices, depending on your desktop environment (such as X11/Wayland active sessions) or a lack of root or input group privileges.


## License

GPL v3.0 or later