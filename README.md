# KrakenExport
KrakenExport is a tool to export staking rewards from [Kraken](https://www.kraken.com) into a csv file. It has been written to be executed twice a week by cron therefor it only queries Kraken for rewards after the last entry in the csv file.

## Dependencies
KrakenExport requires the following libraries:
* OpenSSL (install using `sudo apt-get install libssl-dev` (Ubuntu))
* Curl
* Git (for building)

## Building
1. Create `kraken.key` file, which contains the Kraken API key in the first line and the private key in the second line
2. Create `currencies.txt` file, which contains the list of currencies which shall be querried (one per line; don't forget to add `.S` as Kraken creates an extra staking wallet)
3. Run `cmake CMakeLists.txt` to generate a Makefile
4. Run `make` to build the executable
