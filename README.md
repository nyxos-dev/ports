# NyxOS ports

The package tree for **`xbm`**, the NyxOS package manager. Each package here is
compiled **inside** a running NyxOS by its own `cc` (a ported, self-hosting TinyCC)
and installed to `/mnt/bin`.

This is the modular half of the [NyxOS](https://github.com/nyxos-dev/nyx-os)
distribution: the base OS ships a small set of packages in its initramfs, and this
tree is where the rest live and grow.

## A package

A package is a directory under [`packages/`](packages/) containing its source and a
`recipe`:

```
packages/hello/
├── hello.c      # the source, built with the in-OS cc
└── recipe       # metadata
```

The `recipe` is a few `key: value` lines:

```
name: hello
source: hello.c
bin: hello
```

| key | meaning |
|-----|---------|
| `name` | package name (`xbm install <name>`) |
| `source` | the C file to compile |
| `bin` | the installed binary's name in `/mnt/bin` |
| `url` | *(optional)* an `http://` source to fetch before building |

## Using them in NyxOS

```sh
xbm search              # list available packages
xbm install greet       # compile + install with the in-OS cc
greet                   # run it (installed to /mnt/bin, on the PATH)
xbm remove greet
```

`xbm install` can also pull a source over HTTP when a recipe carries a `url:` field —
the path toward installing straight from this tree.

## Contributing a package

1. Add `packages/<name>/<name>.c` — freestanding C against the NyxOS `libc.h`.
2. Add `packages/<name>/recipe` with `name` / `source` / `bin`.
3. Keep it small and self-contained; it must build with the in-OS `cc`.

## Current catalogue

| Package | What it does |
|---------|--------------|
| `greet` | minimal sample; runs by bare name from `/mnt/bin` |
| `hello` | sample that prints and returns an exit code |
| `ncc`   | a tiny C-adjacent demo built in-OS |
