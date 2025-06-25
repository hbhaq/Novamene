# README

# Overview
This package contains the source code for Novamene: An Efficient Lightweight DoS-Resilient Transaction Pool for Ethereum Clients

# Abstract
The Ethereum transaction pool plays a key role in processing and disseminating live transactions over the network. However, growing transaction loads and spam attacks cause a significant increase in memory consumption of the transaction pool. This not only puts a strain on the resources of Ethereum nodes but also leads to dropped transactions, processing delays, spikes
in transaction fees, and exposes the network to sophisticated attacks. We present Novamene, a novel transaction pool solution for resource-constrained nodes which provides resilience against spam and dust attacks and contributes to the overall health of the network. Novamene reconfigures the transaction pool using bloom filter constructions to fingerprint live transactions as an alternative to maintaining a complete transaction record. We implement Novamene in C++ and benchmark it using a unique 30-day Ethereum transactions dataset. Novamene caters to increased transaction flows with minimal RAM consumption, handling 400 MB worth of transaction load in 4 MB, and processing transactions with 99.99% accuracy. We simulate
congestion and spam events, up to threefold the peak traffic rate witnessed on the Ethereum network, and demonstrate that Novamene maintains high accuracy. Novamene does not require a hard fork, it can be adapted to other cryptocurrencies as a security and optimization solution, and may even be used to build lightweight clients.

## Preprint
Coming soon. 

## Dataset
1. **Txpool State**: https://www.kaggle.com/datasets/txpoolstate/txpool-state

## Included Libraries

The package comes with the following supporting libraries:
- **RapidJSON**: For efficient JSON parsing and manipulation.
- **libbf**: For bloom filter implementation.
- **cppdatetimelite**: For lightweight datetime operations.
- **Helpers Library**: Provides utility functions and transaction structure support for Bitcoin and Ethereum.

## Code Portability

The provided source code is portable and can be adapted to different environments. To run the code:
1. Update the paths to your data files and project directories to match your environment.
2. Ensure all dependencies are correctly configured.

## How to Use

1. Modify the project settings and paths as needed for your environment.
2. Compile and run the project.

For additional information or troubleshooting, refer to the code comments.

## Note

Please ensure that all dependencies are installed and configured before running the projects. If you encounter issues, verify the environment setup and paths are correctly aligned with your system.

https://www.kaggle.com/datasets/txpoolstate/txpool-state
