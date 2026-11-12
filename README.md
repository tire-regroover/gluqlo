# Gluqlo L-Space

A flip clock that sits inside a night landscape. Things happen behind it while it keeps the time.

![Gluqlo L-Space](preview.png)

L-Space is a fork of [Gluqlo](https://github.com/alexanderk23/gluqlo) by Alexander Kovalenko, a SDL remake of the
[Fliqlo screensaver](http://9031.com/goodies/#fliqlo). Gluqlo is itself based on
[noflipqlo](https://github.com/bhm/noflipqlo) (Now Open Flipqlo 2.0) by [Jacek Kuźniarski](https://github.com/bhm).
The flip cards and the flip animation are theirs. Everything behind and around the cards is new in this branch.
It is not affiliated with, or endorsed by, the upstream project.

## What L-Space adds

A landscape behind the cards that changes with the seasons and the weather, plus a lot of small scenes that
turn up now and then, in the sky and on the hills. See `NOTES.md` if you want the details.

## Requirements

- SDL 1.2
- SDL_gfx 1.2
- SDL_ttf 2.0
- XScreensaver (optional)

## Building from source

Install the build-time dependencies (Ubuntu/Debian):

    $ sudo apt-get install build-essential libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libx11-dev

Then compile and install as usual:

    $ make && sudo make install

### Arch Linux

    $ sudo pacman -S --needed base-devel sdl12-compat sdl_ttf sdl_gfx libx11
    $ make && sudo make install

The Ubuntu PPA from the upstream project installs upstream Gluqlo, not L-Space.

## Usage

To use L-Space as a screensaver you may need to remove gnome-screensaver and install XScreensaver (on Ubuntu see
[this page](http://www.howtogeek.com/114027/how-to-add-screensavers-to-ubuntu-12.04/)). Add it to your
`~/.xscreensaver` config file (in the `programs:` section):

    gluqlo -root \n\

Otherwise, you can just run it as is.

Options: `-root` (fullscreen), `-noflip`, `-ampm`, `-leadingzero`, `-w`, `-h`, `-r WxH`, `-s SCALE`,
`-anykeyclose`, `-effect NAME[:KIND]` and `-seed N`. Run `gluqlo -help` for the list.

### Testing effects

Scenery events normally appear at random times. To see one right away, run only that event on repeat:

    $ gluqlo -effect list          # list effect names and their kinds
    $ gluqlo -effect rare_sky      # repeat one effect
    $ gluqlo -effect rare_sky:67   # force a specific kind
    $ gluqlo -seed 42              # start the scenery at a reproducible point

## Repository layout

- `gluqlo.c` – the whole program
- `NOTES.md` – notes on how the code works
- `dev/` – developer tools; not needed to build

## Credits and licence

- Gluqlo © 2014 Alexander Kovalenko; noflipqlo © 2010–2012 Jacek Kuźniarski.
- L-Space © 2026 tire-regroover, written with the help of GitHub Copilot (Anthropic Claude), DeepSeek and Qwen.

Released under the ISC licence, the same as upstream. See `LICENSE`.

## Contributing

1. Fork it!
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request!
