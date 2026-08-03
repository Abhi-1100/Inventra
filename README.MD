# Inventra — Build & Run Instructions

This guide explains how to start the Inventra application. The system consists of two parts that must run simultaneously: the FastAPI Python backend (handles Machine Learning) and the Qt C++ desktop frontend.

---

## 1. Start the FastAPI Backend (Python)

The ML API must be running for the frontend to fetch predictions and recommendations.

1. Open a new terminal.
2. Navigate to the project root directory:
   ```bash
   cd "/Users/ghoriommukeshbai/Desktop/research paper/model/Inventra"
   ```
3. Start the FastAPI server using Uvicorn:
   ```bash
   python3 -m uvicorn main:app --port 8000 --reload
   ```
   *You should see output indicating the server has started and the model is loaded on `http://127.0.0.1:8000`.*

---

## 2. Build the Qt Desktop App (C++)

If you have modified any C++ files (`.cpp` or `.h`), you must recompile the application before running it.

1. Open a second terminal.
2. Navigate to the `build` directory:
   ```bash
   cd "/Users/ghoriommukeshbai/Desktop/research paper/model/Inventra/build"
   ```
3. Run `make` to compile the app (using all available CPU cores for speed):
   ```bash
   make -j$(sysctl -n hw.logicalcpu)
   ```
   *Wait for the `[100%] Built target Inventra` message.*

---

## 3. Run the Qt Desktop App

Once the backend is running and the frontend is built, you can launch the application.

1. In the same terminal (or any terminal in the `build` directory), run:
   ```bash
   open ./Inventra.app
   ```

*The application will launch. The status bar at the bottom should immediately show a green `🟢 API Connected` badge, indicating it successfully connected to your running FastAPI backend.*

---

### Quick Start Reference (Copy & Paste)

**Terminal 1 (Backend):**
```bash
cd "/Users/ghoriommukeshbai/Desktop/research paper/model/Inventra"
python3 -m uvicorn main:app --port 8000 --reload
```

**Terminal 2 (Frontend Build & Run):**
```bash
cd "/Users/ghoriommukeshbai/Desktop/research paper/model/Inventra/build"
make -j$(sysctl -n hw.logicalcpu)
open ./Inventra.app
```