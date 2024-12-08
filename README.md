# AI/ML-Driven Anomaly Detection & RAN Slicing in O-RAN Enabled OAI

## Overview
This repository presents an **AI/ML-powered framework** for anomaly detection and resource allocation in **5G O-RAN networks**, leveraging **OpenAirInterface (OAI)** and **FlexRIC**. The system addresses network intrusion detection, dynamic resource block (RB) allocation, and RRC (Radio Resource Control) release for malicious users in a real-world 5G environment.

Key features include:
- **Anomaly detection** using AI/ML models trained on the [KDDCUP’99 dataset](https://kdd.ics.uci.edu/databases/kddcup99/kddcup99.html).
- **Dynamic resource allocation** via xApps in FlexRIC.
- **Automated RRC UE connection release** for identified attackers.
- Support for multi-slice UPF with embedded anomaly detection functionality.

---

## Anomaly Detection and Resource Allocation Workflow in 5G O-RAN

![image](https://github.com/user-attachments/assets/2e7dd2d6-28c7-4eb1-9e33-792ed0329b1a)


---

## Core Functionalities
1. **Network Intrusion Detection**:
   Utilizes AI/ML models for real-time classification of normal and malicious activities in 5G networks.
2. **Resource Allocation**:
   Dynamically enforces RB allocation based on anomaly scores obtained from UPF-integrated anomaly detection servers.
3. **RAN Slicing**:
   Implements RAN slicing for efficient utilization of 5G resources across multiple user equipment (UE).
4. **DoS Attack Mitigation**:
   Identifies and disconnects UEs generating malicious traffic (e.g., DoS attacks).

---

## Repository Structure
- **`conf/`**: Configuration files for gNB, UE, and multi-UE scripts.
- **`generate-ue-traffic/`**: Scripts to replay KDDCUP’99 traffic data via Scapy for simulating real-world scenarios.
- **`notebooks/`**: Jupyter notebooks for training and evaluating AI/ML models.
- **`slicing-cn/`**: Docker configurations for deploying multi-slice UPFs with integrated anomaly detection servers.
- **`xapp_rc_slice_ctrl.c`**: xApp for RB allocation and RRC release management based on detected anomalies.

---

## Deployment Steps
1. **Core Network Setup**:
   Use Docker Compose to deploy the multi-slice UPF core network.
2. **Compile FlexRIC**:
   Install FlexRIC with the custom xApp (`xapp_rc_slice_ctrl.c`) for slicing and anomaly-triggered RRC release.
3. **Compile OAI**:
   Build OAI components, including gNB and UEs, with telnet library support.
4. **Start nearRT-RIC and RAN Components**:
   Deploy nearRT-RIC, gNB, and UEs.
5. **Simulate UE Traffic**:
   Generate real-world traffic patterns and simulate DoS attacks using Scapy and Hping3.
6. **Monitor Logs**:
   Observe resource allocation and RRC release actions in xApp logs.

---

## Key Use Cases
- **5G Security**: Detect and mitigate network anomalies such as DoS attacks.
- **RAN Slicing**: Enable dynamic resource allocation across slices for efficient network utilization.
- **AI-Driven Decisions**: Integrate machine learning to optimize 5G resource management.

---

## Technical Highlights
- **AI/ML Integration**: Advanced models trained on KDDCUP’99 for real-time intrusion detection.
- **RAN and Core Network Coordination**: Seamless interaction between O-RAN, FlexRIC, and multi-slice UPFs.
- **High Scalability**: Supports real-world 5G scenarios with multiple UEs and slices.

---

## Demo
A demonstration of the system is available here.Please sign in to the drive and then play the video with this link. [Link] (https://drive.google.com/file/d/1XT6Wd-wAtAIFTd2GLLyH0CjzCeVM1CY7/view?usp=sharing)


