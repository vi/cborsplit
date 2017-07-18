# cborsplit

Splits [CBOR][1] data into two streams:

2\. Only text and bytestring contents.  
1\. Everything else.  

```
┌──────┐     *──────────────*     ┌──────────────┐
│ CBOR │ ──> │ cborsplit -s │ ──> │     text     │
└──────┘     *──────────────*     └──────────────┘
               │                    │
               │                    │
               ∨                    ∨
             ┌──────────────┐     *──────────────*     ┌──────┐
             │     misc     │ ──> │ cborsplit -m │ ──> │ CBOR │
             └──────────────┘     *──────────────*     └──────┘
```

```
$ ./cborsplit  --help
Usage:
  cborsplit {-s|--split} in out1 out2
  cborsplit {-m|--merge} in1 in2 out
    '-' instead of in/out means stdin/stdout
    '-' instead of out1/out2/in1/in2 means fd 3/fd 4/fd 3/fd 4

$ ./cborsplit --split something.cbor misc.dat text.dat
$ ./cborsplit --merge misc.dat text.dat something2.cbor
$ cmp something.cbor something2.cbor
```

Why? Maybe to compress something better or to search-and-replace some inside.

Hint: JSON data can be round-tripped though CBOR.

License is MIT + Apache 2.

[1]:https://github.com/cbor/spec-with-errata-fixed/blob/master/rfc7049-errata-corrected.txt
