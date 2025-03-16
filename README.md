# IDCloak


### 1.Run
Fixed the following third-party code
```c++
// span, IDCloak/libs/volePSI/out/install/linux/include/cryptoTools/Common/Matrix.h:176:36: error: no matching function for call to ‘std::span<unsigned char, 18446744073709551615>::span(std::nullptr_t, int)’
    MatrixView<T>::mView = span<T>(nullptr, 0);


// approx
    static  u64 get_bin_size(u64 numBins, u64 numBalls, u64 statSecParam, bool approx = true);
    static  u64 get_bin_size(u64 numBins, u64 numBalls, u64 statSecParam, bool approx = false);
```

Cmake project

```shell
cmake -S . -B build
cmake --build build -j 10
```


### 2.Experiments

```shell
./scripts/run_datasets.sh <type> <mode> [<specific_param>] [<specific_n>]
```

`<type>`: The type of experiment, with the following options:
- psi: cmPSI protocol. (etype == 1)
- fa: secureFA protocol. (etype == 2)

`<mode>`: The mode of the experiment, with the following options:

- lan: Local Area Network mode.
- wan: Wide Area Network mode.
- comm: Communication mode.

`[<specific_param>]` (optional): Specifies a particular parameter set, separated by a space, for example, `"1082 10"`.

`[<specific_n>]` (optional): Specifies a particular `n` value.

```shell
# e.g.
bash ./scripts/run_datasets.sh fs lan
bash scripts/run_datasets.sh psi comm "1700 111" 6
```