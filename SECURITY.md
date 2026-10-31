# Security Policy

## 1. Project Nature & Security Disclaimer

> [!WARNING]
> **Educational & Demonstration Prototype Notice**
> This project is an architectural study and educational replica of the **Zanthus Zeus Retail** Point of Sale (POS) system, written in C/C++ with a Photino.Native HTML5/CSS interface.
> 
> * **Not Certified for Production:** This software is **NOT** homologated or certified by any tax authorities (such as SEFAZ for NFC-e / SAT-CF-e) and is **NOT** compliant with PCI-DSS (Payment Card Industry Data Security Standard).
> * **Simulated Financial Data:** The integrated Electronic Funds Transfer (TEF) / Pinpad module (`zan_tef.c`) operates solely with simulated mocks. Real payment card numbers (PAN), CVVs, track data, or customer PINs must **never** be entered, processed, or stored in this application.
> * **Do Not Use in Commercial Retail:** This system should never be deployed on live retail checkout lanes with real financial transactions.

---

## 2. Supported Versions

Security updates and vulnerability patches are actively maintained for the following branches:

| Version / Branch | Supported | Notes |
| :--- | :---: | :--- |
| `main` | :white_check_mark: | Active development branch; receives immediate fixes |
| `v1.0.x` | :white_check_mark: | Latest stable releases |
| `< 1.0.0` | :x: | Experimental and pre-release commits (EOL) |

If you are using an older build or custom fork, please update to the latest revision of `main` before reporting potential security issues.

---

## 3. Reporting a Vulnerability

We take the security of this project seriously and appreciate responsible disclosure from the community.

### Primary Channel: GitHub Private Vulnerability Reporting (PVR)
If you discover a security vulnerability in this repository, please report it privately using GitHub's built-in tool:

1. Navigate to the **[Security](../../security)** tab of this repository.
2. Under "Security overview", click **[Advisories](../../security/advisories)** (or use **[Report a vulnerability](../../security/advisories/new)**).
3. Complete the advisory form with a detailed summary, severity assessment, reproduction steps, and proof of concept (PoC).

Using Private Vulnerability Reporting allows us to collaborate securely with you in a confidential draft, assess the impact, and develop patches in a private temporary fork before public release.

### Secondary / Fallback Channel: Direct Contact
If you cannot use GitHub's advisory tool or prefer direct communication, you may contact the maintainer via:

* **Security Contact:** `security@estapestudio.com` *(or contact the repository maintainer directly via GitHub profile)*
* **Subject:** `[SECURITY] Potential Vulnerability in Zanthus Zeus PDV Replica`

> [!CAUTION]
> **Do NOT Open Public Issues:** Please do **not** disclose security vulnerabilities through public GitHub Issues, Discussions, or Pull Requests until a patch has been published and coordinated disclosure has been reached.

---

## 4. What to Include in a Security Report

To help us investigate and remediate the issue quickly, please provide:
- **Type of vulnerability** (e.g., buffer overflow, denial of service, memory corruption, path traversal, injection).
- **Affected component(s)** (e.g., `src/core/zan_barcode.c`, `src/core/zan_db.c`, embedded `cpp-httplib` server, Photino IPC bridge).
- **Environment details:** Operating system (Windows 11, Debian Linux), architecture (x64), compiler, and commit hash.
- **Step-by-step reproduction instructions:** Minimal steps required to trigger the issue.
- **Proof of Concept (PoC):** Sample input string, network payload, or script demonstrating the vulnerability.
- **Estimated impact:** What an attacker could achieve if the vulnerability were exploited.

---

## 5. Response Timeframes & SLAs

We are committed to handling security vulnerabilities in a timely and transparent manner:

| Phase | Target Timeline |
| :--- | :--- |
| **Initial Acknowledgement** | Within **48 hours** |
| **Triage & Vulnerability Assessment** | Within **7 calendar days** |
| **Fix Development & Patch Release** | Within **30 calendar days** |
| **Coordinated Public Disclosure** | Within **90 days** or upon release of the fix |

You will be kept informed of our progress throughout the triage and patching cycle, and credited in the release notes and advisory (unless you request anonymity).

---

## 6. Architecture & Domain-Specific Security Guidelines

When reviewing or contributing to this codebase, keep in mind the following security invariants:

1. **Loopback Server Isolation:**
   - The embedded HTTP server (`cpp-httplib`) utilized by the desktop app must bind strictly to `127.0.0.1` (loopback) and never to `0.0.0.0` or public network interfaces. This prevents unauthorized nodes on the local network (LAN/Wi-Fi) from interacting with `/api/command` or extracting cash register telemetry.
2. **Buffer Bounds & Input Validation (C Core):**
   - All external inputs from barcode scanners (USB HID/wedge), electronic scale RS-232 serial interfaces, and operator keyboard entry must be strictly checked against buffer boundaries to prevent buffer overflows or memory corruption.
3. **Database Injection Protections:**
   - Database operations in `src/core/zan_db.c` must utilize SQLite parameterized prepared statements (`sqlite3_bind_*`) rather than string concatenation to prevent SQL injection.
4. **Credential & Secret Protection:**
   - Default operator credentials (e.g., operator `101`, password `1234`) are provided for testing purposes only. Production credentials, digital certificates (e.g., e-CNPJ A1 `.pfx` certificates), and private keys must never be committed to this repository.

