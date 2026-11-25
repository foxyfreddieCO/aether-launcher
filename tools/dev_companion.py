#!/usr/bin/env python3
"""
Aether Launcher - Local Development & Auto-Update Companion (Windows 11)

Watches a local Git clone of Aether Launcher, pulls approved commits pushed by
ChatGPT / GitHub workflows or local edits, runs CMake build + CTest unit tests,
packages a verified ZIP archive with SHA-256 hash, and serves a local loopback
update feed at http://127.0.0.1:47821/manifest.json.

Point Aether Launcher -> Settings -> Updates -> Update Manifest Feed URL to:
    http://127.0.0.1:47821/manifest.json
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import http.server
import json
import os
import shutil
import socketserver
import subprocess
import sys
import threading
import time
import zipfile
from pathlib import Path


class CompanionState:
    def __init__(self, repo_dir: Path, build_dir: Path, dist_dir: Path, qt_dir: str, port: int):
        self.repo_dir = repo_dir.resolve()
        self.build_dir = build_dir.resolve()
        self.dist_dir = dist_dir.resolve()
        self.qt_dir = qt_dir
        self.port = port
        self.lock = threading.Lock()
        self.manifest: dict = {}
        self.last_commit: str = ""
        self.build_counter: int = 0
        self.dist_dir.mkdir(parents=True, exist_ok=True)

    def current_commit(self) -> str:
        res = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=self.repo_dir,
            capture_output=True,
            text=True,
            check=False,
        )
        return res.stdout.strip()

    def git_pull_if_remote(self) -> bool:
        fetch = subprocess.run(
            ["git", "fetch", "--all", "--prune"],
            cwd=self.repo_dir,
            capture_output=True,
            text=True,
            check=False,
        )
        if fetch.returncode != 0:
            return False
        pull = subprocess.run(
            ["git", "pull", "--ff-only"],
            cwd=self.repo_dir,
            capture_output=True,
            text=True,
            check=False,
        )
        return pull.returncode == 0

    def build_test_and_package(self) -> bool:
        next_version = f"1.0.{self.build_counter + 1}"
        print(f"[companion] Starting build & test cycle v{next_version} in {self.repo_dir}...")
        cfg_cmd = [
            "cmake",
            "-B",
            str(self.build_dir),
            "-DAETHER_BUILD_TESTS=ON",
            f"-DAETHER_VERSION={next_version}",
        ]
        if self.qt_dir:
            cfg_cmd.append(f"-DCMAKE_PREFIX_PATH={self.qt_dir}")

        if subprocess.run(cfg_cmd, cwd=self.repo_dir).returncode != 0:
            print("[companion] CMake configure failed; keeping previous verified package.")
            return False

        build_cmd = ["cmake", "--build", str(self.build_dir), "--config", "Release", "--parallel"]
        if subprocess.run(build_cmd, cwd=self.repo_dir).returncode != 0:
            print("[companion] Build failed; keeping previous verified package.")
            return False

        test_cmd = ["ctest", "--test-dir", str(self.build_dir), "-C", "Release", "--output-on-failure"]
        if subprocess.run(test_cmd, cwd=self.repo_dir).returncode != 0:
            print("[companion] Unit tests failed; aborting release package.")
            return False

        stage_dir = self.dist_dir / "stage"
        if stage_dir.exists():
            shutil.rmtree(stage_dir)
        stage_dir.mkdir(parents=True, exist_ok=True)

        exe_suffix = ".exe" if os.name == "nt" else ""
        candidates = [
            self.build_dir / "Release",
            self.build_dir,
        ]
        bin_root = next((c for c in candidates if (c / f"AetherLauncher{exe_suffix}").exists()), None)
        if bin_root is None:
            print("[companion] Could not find compiled AetherLauncher binary.")
            return False

        for name in (f"AetherLauncher{exe_suffix}", f"AetherUpdater{exe_suffix}"):
            src = bin_root / name
            if src.exists():
                shutil.copy2(src, stage_dir / name)

        if os.name == "nt" and self.qt_dir:
            windeploy = Path(self.qt_dir) / "bin" / "windeployqt.exe"
            if windeploy.exists():
                subprocess.run(
                    [
                        str(windeploy),
                        "--release",
                        "--qmldir",
                        str(self.repo_dir / "src" / "ui" / "qml"),
                        str(stage_dir / "AetherLauncher.exe"),
                    ],
                    check=False,
                )

        pkg_path = self.dist_dir / "package.zip"
        with zipfile.ZipFile(pkg_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
            for item in stage_dir.rglob("*"):
                if item.is_file():
                    zf.write(item, item.relative_to(stage_dir))

        sha = hashlib.sha256(pkg_path.read_bytes()).hexdigest()
        size = pkg_path.stat().st_size

        notes_res = subprocess.run(
            ["git", "log", "-n", "3", "--pretty=format:- %s"],
            cwd=self.repo_dir,
            capture_output=True,
            text=True,
            check=False,
        )
        self.build_counter += 1
        version = f"1.0.{self.build_counter}"

        manifest = {
            "version": version,
            "releaseDate": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "releaseNotes": notes_res.stdout.strip() or "Local verified companion build.",
            "downloadUrl": f"http://127.0.0.1:{self.port}/package.zip",
            "sha256": sha,
            "fileSizeBytes": size,
        }

        with self.lock:
            self.manifest = manifest
            self.last_commit = self.current_commit()
            (self.dist_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

        print(f"[companion] Verified build v{version} ready! SHA-256: {sha[:16]}...")
        return True


def make_handler(state: CompanionState):
    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path == "/manifest.json":
                with state.lock:
                    payload = json.dumps(state.manifest, indent=2).encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(payload)))
                self.end_headers()
                self.wfile.write(payload)
                return

            if self.path == "/package.zip":
                pkg = state.dist_dir / "package.zip"
                if not pkg.exists():
                    self.send_error(404, "No package built yet")
                    return
                data = pkg.read_bytes()
                self.send_response(200)
                self.send_header("Content-Type", "application/zip")
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)
                return

            self.send_error(404)

        def log_message(self, format, *args):
            print(f"[http] {format % args}")

    return Handler


def main() -> int:
    parser = argparse.ArgumentParser(description="Aether Launcher Local Update Companion")
    parser.add_argument("--repo", default=".", help="Path to local AetherLauncher Git repository")
    parser.add_argument("--build-dir", default="build-companion", help="Build directory")
    parser.add_argument("--dist-dir", default="dist-companion", help="Packaged update output directory")
    parser.add_argument("--qt-dir", default=os.environ.get("Qt6_DIR", ""), help="Qt6 install prefix (e.g. C:/Qt/6.8.2/msvc2022_64)")
    parser.add_argument("--port", type=int, default=47821, help="Loopback HTTP port (default 47821)")
    parser.add_argument("--poll-seconds", type=int, default=30, help="Git poll interval in seconds")
    parser.add_argument("--no-initial-build", action="store_true", help="Skip building immediately on start")
    args = parser.parse_args()

    state = CompanionState(
        repo_dir=Path(args.repo),
        build_dir=Path(args.build_dir),
        dist_dir=Path(args.dist_dir),
        qt_dir=args.qt_dir,
        port=args.port,
    )

    if not args.no_initial_build:
        state.build_test_and_package()

    def watcher():
        while True:
            time.sleep(max(5, args.poll_seconds))
            state.git_pull_if_remote()
            head = state.current_commit()
            if head and head != state.last_commit:
                print(f"[companion] New commit detected ({head[:8]}), rebuilding...")
                state.build_test_and_package()

    threading.Thread(target=watcher, daemon=True).start()

    # Bind strictly to 127.0.0.1 loopback for security
    with socketserver.TCPServer(("127.0.0.1", args.port), make_handler(state)) as httpd:
        print(f"[companion] Serving local update feed at http://127.0.0.1:{args.port}/manifest.json")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[companion] Shutting down.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
