AI/ML-Driven Anomaly Detection & RAN Slicing in O-RAN Enabled OAI
Overview
This repository presents an AI/ML-powered framework for anomaly detection and resource allocation in 5G O-RAN networks, leveraging OpenAirInterface (OAI) and FlexRIC. The system addresses network intrusion detection, dynamic resource block (RB) allocation, and RRC (Radio Resource Control) release for malicious users in a real-world 5G environment.

Key features include:

Anomaly detection using AI/ML models trained on the KDDCUP’99 dataset.
Dynamic resource allocation via xApps in FlexRIC.
Automated RRC UE connection release for identified attackers.
Support for multi-slice UPF with embedded anomaly detection functionality.
Core Functionalities
Network Intrusion Detection: Utilizes AI/ML models for real-time classification of normal and malicious activities in 5G networks.
Resource Allocation: Dynamically enforces RB allocation based on anomaly scores obtained from UPF-integrated anomaly detection servers.
RAN Slicing: Implements RAN slicing for efficient utilization of 5G resources across multiple user equipment (UE).
DoS Attack Mitigation: Identifies and disconnects UEs generating malicious traffic (e.g., DoS attacks).
Repository Structure
conf/: Configuration files for gNB, UE, and multi-UE scripts.
generate-ue-traffic/: Scripts to replay KDDCUP’99 traffic data via Scapy for simulating real-world scenarios.
notebooks/: Jupyter notebooks for training and evaluating AI/ML models.
slicing-cn/: Docker configurations for deploying multi-slice UPFs with integrated anomaly detection servers.
xapp_rc_slice_ctrl.c: xApp for RB allocation and RRC release management based on detected anomalies.
Deployment Steps
Core Network Setup:
Use Docker Compose to deploy the multi-slice UPF core network.
Compile FlexRIC:
Install FlexRIC with the custom xApp (xapp_rc_slice_ctrl.c) for slicing and anomaly-triggered RRC release.
Compile OAI:
Build OAI components, including gNB and UEs, with telnet library support.
Start nearRT-RIC and RAN Components:
Deploy nearRT-RIC, gNB, and UEs.
Simulate UE Traffic:
Generate real-world traffic patterns and simulate DoS attacks using Scapy and Hping3.
Monitor Logs:
Observe resource allocation and RRC release actions in xApp logs.
Key Use Cases
5G Security: Detect and mitigate network anomalies such as DoS attacks.
RAN Slicing: Enable dynamic resource allocation across slices for efficient network utilization.
AI-Driven Decisions: Integrate machine learning to optimize 5G resource management.
Technical Highlights
AI/ML Integration: Advanced models trained on KDDCUP’99 for real-time intrusion detection.
RAN and Core Network Coordination: Seamless interaction between O-RAN, FlexRIC, and multi-slice UPFs.
High Scalability: Supports real-world 5G scenarios with multiple UEs and slices.
Citation
If you find this work useful, please cite:

bibtex
Copy code
@inproceedings{10.1145/3636534.3697311,
  author = {Tsourdinis, Theodoros and Makris, Nikos and Korakis, Thanasis and Fdida, Serge},
  title = {AI-Driven Network Intrusion Detection and Resource Allocation in Real-World O-RAN 5G Networks},
  year = {2024},
  isbn = {9798400704895},
  publisher = {Association for Computing Machinery},
  address = {New York, NY, USA},
  url = {https://doi.org/10.1145/3636534.3697311},
  doi = {10.1145/3636534.3697311},
  booktitle = {Proceedings of the 30th Annual International Conference on Mobile Computing and Networking},
  pages = {1842--1849},
  numpages = {8},
  keywords = {5G, O-RAN, security, AI/ML, slicing, OAI, OpenAirInterface, FlexRIC},
  location = {Washington D.C., DC, USA},
  series = {ACM MobiCom '24}
}
Demo
A demonstration of the system is available on YouTube.

Contributing
Contributions to enhance the framework are welcome. Please refer to the README.md for setup instructions. For issues, create a ticket in the repository.

Focus on building secure, efficient, and AI-driven 5G networks with this framework.












