# Toralize

Toralize is a command-line tool that routes curl requests through the Tor network, providing anonymity and privacy for your web requests.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Installation](#installation)
4. [Usage](#usage)
5. [Examples](#examples)
6. [Troubleshooting](#troubleshooting)
7. [Limitations](#limitations)
8. [Contributing](#contributing)

## Overview

Toralize is a bash script that wraps curl commands, routing them through the Tor network. This allows you to make anonymous web requests, access .onion sites, and bypass certain geographic restrictions.

## Prerequisites

- Bash shell
- curl
- Tor service installed and running
- netcat (nc) for checking Tor proxy status

## Installation

1. Clone this repository or download the `toralize_curl` script.
2. Move the script to a directory in your PATH, for example:

```bash
sudo mv toralize_curl /usr/local/bin/
```

3. Make the script executable:

```bash
sudo chmod +x /usr/local/bin/toralize_curl
```

4. Ensure Tor is installed and running on your system.

## Usage

The basic syntax for using toralize_curl is:

```bash
toralize_curl [curl options] [URL]
```

If no URL is provided, toralize_curl will return your Tor IP address.

## Examples

1. Check your Tor IP address:
   ```bash
   toralize_curl
   ```

2. Make a GET request to a website:
   ```bash
   toralize_curl https://example.com
   ```

3. Save output to a file:
   ```bash
   toralize_curl -o output.html https://example.com
   ```

4. Use verbose output:
   ```bash
   toralize_curl -v https://example.com
   ```

5. Access a .onion site:
   ```bash
   toralize_curl http://2gzyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid.onion
   ```

## Troubleshooting

- If you receive a "403 Forbidden" error, the website may be blocking Tor exit nodes. Try a different website or wait and try again later.
- Ensure the Tor service is running on your system. You can typically start it with:
  ```bash
  sudo service tor start
  ```
- If you're having issues, try running toralize_curl with the -v flag for verbose output, which may provide more information about the problem.

## Limitations

- Some websites may block or limit access from Tor exit nodes.
- The anonymity provided by Tor comes at the cost of slower connection speeds.
- Not all web services or protocols may work correctly through Tor.

## Contributing

Contributions to improve toralize are welcome. Please feel free to submit pull requests or open issues to suggest improvements or report bugs.

---

Remember to use Tor responsibly and in accordance with local laws and regulations. While Tor provides anonymity, it's not foolproof, and users should be aware of its limitations and potential risks.
