# B. Role Source Archive

Status: durable transcription and traceability record
Date: 2026-07-26
Inputs: one CV PDF, nine local image files including one byte-identical duplicate, and one inline role screenshot supplied in the review request

## Purpose

This file makes future planning independent of the original screenshots. It preserves every substantive responsibility, qualification, differentiator, and credential from the supplied material, then maps it to the canonical `PGE-*` requirements in [A. Principal Graphics Engineering Requirements](Requirements.md).

This is a normalized transcription, not a legal copy of a job advertisement:

- employer names, slogans, requisition numbers, locations, and application controls are omitted;
- named employers are replaced with neutral phrases such as “architecture team,” “external game developer,” or “hardware partner”;
- generic technology names such as D3D12, Vulkan, HLSL, CUDA, HIP, ONNX, PyTorch, PIX, and RenderDoc are retained because they are actual skill requirements;
- obvious spelling and OCR errors are corrected;
- duplicated screenshots are recorded once;
- no phone number, personal email address, consent clause, or other unnecessary personal data from the CV is stored here.

## Input Inventory

| Source | Local input | Content | Treatment |
| --- | --- | --- | --- |
| `S1` | Inline screenshot in the review request | Principal advanced rendering, path tracing, AI, architecture/driver collaboration | Transcribed below. |
| `S2` | `image (1).webp` | Graphics workload-analysis tools | Transcribed below. |
| `S3` | `image (2).webp` and `image (5).webp` | Graphics software development | The second image is the legible continuation/repetition of the first. Merged once. |
| `S4` | `image (3).webp` and `image (4).webp` | Neural-rendering research inference | Merged across the two screenshots. |
| `S5` | `image (6).webp`, `image (7).webp`, and `image (8).webp` | Advanced rendering research productization | `image (6)` and `image (7)` are byte-identical; duplicate removed. `image (8)` is the continuation. |
| `S6` | `image.webp` | Short public hiring summary for a graphics role | Transcribed below. |
| `CV1` | `PawelStoleckiCV.pdf` | Candidate CV snapshot | Structured evidence summary below. |

File hashes recorded during intake confirm that `image (6).webp` and `image (7).webp` are identical. The remaining supplied images are distinct.

## S1. Principal Advanced Rendering And AI Adoption

### Role focus

- Focus on advanced rendering, including path tracing and neural graphics.
- Drive broader adoption of AI technology in games.

### Responsibilities

- Collaborate with highly innovative AAA developers around the world to optimize GPU and system performance and deliver fast, fluid gameplay with path-traced neural rendering and complex AI behaviors. (`PGE-01`, `PGE-02`, `PGE-03`, `PGE-05`, `PGE-14`, `PGE-15`)
- Develop, profile, optimize, and tune neural-rendering algorithms and AI models to make effective use of available compute resources. (`PGE-03`–`PGE-05`, `PGE-10`–`PGE-12`)
- Collaborate with architecture and driver teams to ensure the best experience on current hardware and help determine trends and features for future architectures. (`PGE-01`, `PGE-05`, `PGE-06`, `PGE-10`, `PGE-15`)
- Explore current GPU technology, develop new GPU techniques, build technical demonstrations, write whitepapers, and present work at conferences. (`PGE-08`, `PGE-10`, `PGE-13`, `PGE-15`)

### Required background

- Master’s degree in computer science, computer engineering, or a related computationally focused science, or equivalent experience. (`PGE-08`, `PGE-15`)
- Fifteen or more years of relevant work experience or research. (`PGE-15`)
- Proficiency in C++ with strong software development, optimization, debugging, and software-engineering foundations. (`PGE-05`–`PGE-07`, `PGE-10`)
- Very strong mathematics, including linear algebra and calculus, for problem solving and performance modeling. (`PGE-08`)
- Excellent real-time graphics and GPU knowledge, including real-time rendering algorithms, ray/path tracing, shaders, shading languages, and APIs such as DirectX or Vulkan. (`PGE-02`, `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10`)
- Hands-on low-level performance optimization experience. (`PGE-05`, `PGE-06`, `PGE-10`)
- In-depth CPU and GPU architecture fundamentals. (`PGE-05`, `PGE-10`)
- Solid AI fundamentals, including the ability to use AI tools for programming productivity and to design new solutions. (`PGE-07`, `PGE-11`)
- Good communication, organization, and prioritization. (`PGE-01`, `PGE-13`, `PGE-15`)
- Willingness to travel for on-site developer visits and conferences. (`PGE-01`, `PGE-13`, `PGE-14`)

### Differentiators

- Professional experience with machine-learning algorithms and applications. (`PGE-03`, `PGE-04`, `PGE-11`, `PGE-12`)
- Experience optimizing inference and training workloads. (`PGE-04`, `PGE-05`, `PGE-10`–`PGE-12`)
- Solid understanding of neural-graphics techniques. (`PGE-03`, `PGE-04`, `PGE-08`, `PGE-11`)
- Windows or Linux driver-development experience. (`PGE-06`, `PGE-10`, `PGE-14`)

## S2. Graphics Workload-Analysis Tools

### Responsibilities

- Design and develop graphics tools that analyze and process D3D12 and Vulkan workloads. (`PGE-06`, `PGE-07`, `PGE-09`, `PGE-13`)
- Develop techniques for analyzing how different game engines use graphics APIs. (`PGE-05`, `PGE-06`, `PGE-09`, `PGE-13`)
- Investigate and resolve complex graphics issues using deep expertise in both APIs. (`PGE-06`, `PGE-09`)
- Collaborate with other engineers to integrate new features and enhancements into existing systems. (`PGE-01`, `PGE-07`, `PGE-13`)
- Maintain high-quality documentation and coding standards. (`PGE-07`, `PGE-13`, `PGE-15`)
- Collaborate closely with multiple teams to deliver solutions that work for them. (`PGE-01`, `PGE-15`)
- Contribute to future architecture for a highly scalable, durable, and innovative system. (`PGE-07`, `PGE-10`, `PGE-15`)

### Preferred experience

- Proficiency in C++ and Python. (`PGE-07`)
- In-depth D3D12 and/or Vulkan knowledge and experience. (`PGE-06`, `PGE-09`)
- Experience with graphics debuggers such as PIX and RenderDoc. (`PGE-06`, `PGE-14`)
- Solid understanding of rasterization, ray-tracing pipelines, and GPU architecture. (`PGE-02`, `PGE-09`, `PGE-10`)
- Game-development or real-time-rendering experience. (`PGE-02`, `PGE-05`, `PGE-09`)
- Familiarity with CPU and GPU performance-profiling tools and techniques. (`PGE-05`, `PGE-06`, `PGE-10`)
- Excellent communication and collaborative teamwork. (`PGE-01`, `PGE-13`, `PGE-15`)
- Passion for graphics technology and AI with continuous learning. (`PGE-11`, `PGE-13`, `PGE-15`)
- Experience with machine-learning frameworks such as TensorFlow, PyTorch, or similar. (`PGE-11`, `PGE-12`)

## S3. Graphics Software Development

### Persona

- A strategic graphical engineer passionate about creating new GPU-rendered experiences.
- Work with current hardware and software technology to push rendering boundaries.
- Combine graphics and software engineering with innovation and research.
- Communicate effectively and work well with different teams.

### Responsibilities

- Develop and maintain applications using DirectX 12, Vulkan, and other modern graphics APIs. (`PGE-07`, `PGE-09`, `PGE-14`)
- Optimize complex algorithms and solve difficult problems to improve software efficiency and performance. (`PGE-05`, `PGE-07`, `PGE-08`, `PGE-10`)
- Design and implement advanced shaders for high-quality visual effects. (`PGE-02`, `PGE-08`, `PGE-09`)
- Collaborate across functions to deliver integrated solutions and execute projects smoothly. (`PGE-01`, `PGE-13`, `PGE-15`)
- Innovate and implement ray-tracing technologies, especially global illumination or path-tracing applications. (`PGE-02`, `PGE-08`, `PGE-09`)
- Develop tools and plugins that support developers and streamline graphics programming. (`PGE-01`, `PGE-06`, `PGE-07`, `PGE-13`)
- Explore and apply machine-learning techniques to optimize graphics processing and performance. (`PGE-03`–`PGE-05`, `PGE-11`, `PGE-12`)

### Essentials

- Extensive C++ and graphics-programming experience with high software-development proficiency. (`PGE-07`, `PGE-09`, `PGE-15`)
- Expertise writing and optimizing complex shaders for graphics applications. (`PGE-05`, `PGE-09`, `PGE-10`)
- Successful cross-functional collaboration with strong interpersonal and communication skills. (`PGE-01`, `PGE-13`, `PGE-15`)

### Useful differentiators

- Hands-on ray tracing, especially global illumination or path tracing. (`PGE-02`)
- Experience creating developer tools or plugins that improve functionality and usability. (`PGE-06`, `PGE-07`, `PGE-13`)
- Understanding of machine-learning applications in graphics optimization and use of AI techniques to improve graphics processing. (`PGE-03`, `PGE-04`, `PGE-11`, `PGE-12`)

### Academic foundation

- Bachelor’s or master’s degree in computer science or a related field, with grounding in graphics, mathematics, machine learning, or computer engineering. Equivalent relevant experience is also represented elsewhere in the source set. (`PGE-08`, `PGE-11`, `PGE-15`)

## S4. Neural-Rendering Research Inference

### Persona and role

- Translate neural-network models and algorithms into efficient inference solutions.
- Understand graphics and machine learning, neural-network operators, their mathematical foundations, and computational requirements.
- Use C++, HIP, CUDA, HLSL, and related GPU technologies.
- Communicate and work across teams to create optimized inference implementations for neural rendering and generative-AI applications.
- Implement and optimize high-performance GPU kernels for machine-learning operators.
- Work with researchers developing real-time neural graphics, inverse rendering, and other ML-accelerated opportunities.

### Responsibilities

- Work with ML researchers and engineers to translate models and algorithms written in PyTorch/ONNX into efficient GPU shaders using HIP, CUDA, HLSL, or equivalent languages. (`PGE-01`, `PGE-03`, `PGE-04`, `PGE-09`, `PGE-11`, `PGE-12`)
- Design, implement, and optimize high-performance GPU kernels for ML operators. (`PGE-04`, `PGE-05`, `PGE-10`, `PGE-12`)
- Work across research, hardware, driver, and compiler teams to analyze performance issues and improve rendering speed and ML-workload efficiency. (`PGE-01`, `PGE-05`, `PGE-06`, `PGE-10`, `PGE-12`, `PGE-15`)
- Stay current with GPU hardware, rendering techniques, graphics APIs, and GPU-accelerated ML. (`PGE-02`, `PGE-09`–`PGE-13`)
- Contribute tools and methodologies for integrating optimized shaders into game engines. (`PGE-01`, `PGE-04`, `PGE-06`, `PGE-07`, `PGE-13`)
- Document and share best practices for graphics and compute/ML GPU programming. (`PGE-07`, `PGE-13`, `PGE-15`)
- Participate in code reviews and provide constructive feedback. (`PGE-01`, `PGE-07`, `PGE-15`)

### Preferred experience

- Strong object-oriented programming; C or C++ preferred. (`PGE-07`)
- Proven ML GPU-kernel development and optimization using HIP, OpenCL, CUDA, HLSL, or equivalent. (`PGE-04`, `PGE-05`, `PGE-09`, `PGE-10`, `PGE-12`)
- Ability to program or reason in low-level languages such as x86 assembly, SIMD intrinsics, GPU ISA, or comparable assembly. (`PGE-10`)
- Strong GPU-architecture knowledge: compute cores, cache hierarchy, memory model, graphics APIs, and shader programming. (`PGE-05`, `PGE-09`, `PGE-10`)
- Solid understanding of common neural-network operators, mathematics, and computational requirements. (`PGE-04`, `PGE-08`, `PGE-11`)
- Modern concurrent-programming and threading APIs. (`PGE-07`, `PGE-10`)
- Windows and Linux operating-system development. (`PGE-14`)
- Software-development processes and tools such as debuggers, Git-based source control, and profilers. (`PGE-06`, `PGE-07`, `PGE-14`)
- Machine-learning techniques and graphics applications. (`PGE-03`, `PGE-04`, `PGE-11`, `PGE-12`)
- Effective communication, problem solving, and interpersonal skills. (`PGE-01`, `PGE-13`, `PGE-15`)

## S5. Advanced Rendering Research Productization

### Persona and operating context

- Work at the intersection of graphics research and productization.
- Create cutting-edge software that equips game developers and publishers to build immersive, high-performance games.
- Bring a strong technical background in GPU/APU programming, C++, and machine learning.
- Collaborate within and across teams and synthesize information from specialist domains.
- Influence technical direction and start new projects from first principles.

### Responsibilities

- Work closely with research engineers to transform proof-of-concept prototypes into mature, high-quality products. (`PGE-01`, `PGE-03`, `PGE-07`, `PGE-13`, `PGE-15`)
- Collaborate with external game-development partners to integrate technology into their titles. (`PGE-01`, `PGE-13`, `PGE-15`)
- Optimize, extend, package, and document high-level compute-shader and C++ code. (`PGE-04`–`PGE-07`, `PGE-09`, `PGE-10`, `PGE-13`)

### Required experience

- Computer-science background and at least five years of full-time experience writing efficient high-level shader code such as HLSL SM6, GLSL, or Slang and modern C++17 or newer. (`PGE-07`, `PGE-09`, `PGE-15`)
- Strong low-level machine-learning concepts and design patterns including automatic differentiation, computational graphs, tensor broadcasting, and related foundations. (`PGE-04`, `PGE-08`, `PGE-11`)
- Real-time-rendering and graphics-algorithm knowledge. (`PGE-02`, `PGE-05`, `PGE-08`, `PGE-09`)
- Excellent written and verbal English. (`PGE-13`, `PGE-14`)
- Willingness to travel domestically and internationally when required. (`PGE-01`, `PGE-14`)

### Desirable but not essential

- Knowledge of modern GPU and game-console instruction-set architectures. (`PGE-10`, `PGE-14`)
- Experience with modern ML libraries such as PyTorch or TensorFlow. (`PGE-11`, `PGE-12`)
- Physically based rendering algorithms: sampling, shading, and light transport. (`PGE-02`, `PGE-08`, `PGE-09`)
- Modern graphics APIs such as D3D12 or Vulkan. (`PGE-06`, `PGE-09`)
- Applied mathematics in a relevant field: linear algebra, differential calculus, stochastic optimization, and statistical analysis. (`PGE-08`, `PGE-11`)

### Academic foundation

- PhD, bachelor’s, or master’s degree emphasizing computer science, computer engineering, or applied mathematics with relevant experience preferred. (`PGE-08`, `PGE-15`)

## S6. Short Public Hiring Summary

- D3D12 and Vulkan APIs. (`PGE-06`, `PGE-09`)
- C++ and Python. (`PGE-07`)
- Real-time rendering, rasterization, and ray tracing. (`PGE-02`, `PGE-05`, `PGE-09`)
- Graphics debuggers including PIX and RenderDoc. (`PGE-06`, `PGE-14`)

This summary duplicates a subset of `S2`; it is retained because it shows which four skills were selected for the public-facing hiring message.

## CV1. Candidate Evidence Snapshot

The PDF is a one-page CV. Text extraction is visually ordered rather than semantically ordered, so the content below is normalized into sections.

### Professional identity and summary

- Current identity: rendering engineer.
- Specialization: real-time physically based light transport, denoising, advanced material systems, low-level GPU optimization, memory management, and parallelism.
- Claimed core skills: HLSL, C++, rendering hardware interfaces, GPU architecture, shader development, pipeline profiling, cross-platform scalability, and hardware-aware performance tuning.
- Product context: large open worlds and AA/AAA titles using proprietary and commercial engines.

### Experience chronology

- Rendering engineer, 2025–present:
  - delivered a 60 FPS real-time GI optimization target for a current-generation console technology demonstration;
  - leads internal development of the GI system;
  - coordinates advanced lighting features with external engine and hardware partners;
  - performs low-level GPU profiling and cross-platform performance optimization.
- Senior technical-art engineer, 2024–2025.
- Technical-art engineer, 2023–2024.
- Rendering programmer / technical artist, 2021–2023:
  - improved direct, indirect, and volumetric lighting;
  - optimized GPU, CPU, and memory performance across platforms;
  - drove scalability from an older console generation to high-end PC.
- Junior technical artist, 2019–2021:
  - developed advanced GBuffer and post-processing shaders;
  - performed cross-platform profiling and optimization;
  - adapted a rendering pipeline for a handheld console.
- 3D pipeline instructor, 2019:
  - trained a team on environment workflows, shader creation, optimization, and rendering-pipeline cost.
- Technical artist roles, 2017–2019:
  - built landscape pipelines, procedural fire/ocean shaders, foliage wind simulation, general shaders, and pipeline tools.

### Public products and communication

- Credits include a current open-world project and technology demonstration, a shipped open-world game and expansion, a shipped handheld port, and a shipped multiplayer title.
- Talks and publications listed from 2018–2025 cover engine programming, physically based rendering, shipped-game lighting, technical knowledge for artists, natural structures, landscape construction, and the technical-artist role.
- Education lists a leadership program and a bachelor’s degree; the extracted PDF does not state the bachelor’s field clearly enough to claim a computational focus.

### CV limitations visible from the artifact

- Most roles have no outcome bullets; the strongest measurable evidence is concentrated in the current and 2019–2023 roles.
- D3D12, Vulkan, Python, PIX, RenderDoc, machine learning, neural graphics, Linux, and named shader/compiler targets are not explicit.
- “Publications” mixes conference talks, community talks, and articles; these should be separated accurately.
- The PDF’s typography inserts spaces between characters during text extraction, which is an ATS and accessibility risk.
- The profile URL embedded in this PDF differs from the current public-profile URL supplied for this review.

## Traceability Closure

The following checklist prevents future source loss:

- [x] Every `S1` responsibility, qualification, differentiator, and travel expectation is mapped.
- [x] Every `S2` responsibility and preferred qualification is mapped.
- [x] Every `S3` responsibility, essential, differentiator, and academic expectation is mapped.
- [x] Every `S4` responsibility and preferred qualification visible across both screenshots is mapped.
- [x] Every `S5` responsibility, required qualification, differentiator, and academic expectation is mapped.
- [x] Every `S6` public-summary skill is mapped.
- [x] The CV’s professional claims, roles, products, talks, education, and visible presentation problems are preserved without contact data.

Future source material should be added here first, then reconciled against `PGE-01` through `PGE-15`. Add a new canonical requirement only when the new expectation cannot honestly fit an existing ID.
