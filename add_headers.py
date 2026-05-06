#!/usr/bin/env python3
import os


class FilePathAnnotator:
    def __init__(self, root="."):
        self.root = os.path.abspath(root)
        self.extensions = (".c", ".h")
        self.ignored_dirs = {"build", ".git", "bin","generated_files"}

    # ============================================================
    # BUILD EXPECTED HEADER
    # ============================================================

    def get_header(self, file_path):
        rel_path = os.path.relpath(file_path, self.root)
        return f"// ./{rel_path}"

    # ============================================================
    # CLEAN + ANNOTATE FILE (IDEMPOTENT)
    # ============================================================

    def annotate_file(self, file_path):
        try:
            with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                lines = f.readlines()
        except:
            return

        header = self.get_header(file_path)

        # --------------------------------------------------------
        # STEP 1: REMOVE ALL EXISTING DUPLICATE HEADERS AT TOP
        # --------------------------------------------------------
        i = 0
        while i < len(lines) and lines[i].strip().startswith("// ./"):
            i += 1

        remaining = lines[i:]

        # --------------------------------------------------------
        # STEP 2: BUILD CLEAN CONTENT (NO EXTRA BLANK LINE)
        # --------------------------------------------------------
        new_lines = [header + "\n"] + remaining

        # --------------------------------------------------------
        # STEP 3: SKIP IF NO CHANGE (LINT BEHAVIOR)
        # --------------------------------------------------------
        if lines == new_lines:
            return

        # --------------------------------------------------------
        # STEP 4: WRITE BACK
        # --------------------------------------------------------
        with open(file_path, "w", encoding="utf-8") as f:
            f.writelines(new_lines)

        print(f"✅ Fixed: {file_path}")

    # ============================================================
    # WALK PROJECT
    # ============================================================

    def run(self):
        print("🔍 Linting file path headers...\n")

        for root, dirs, files in os.walk(self.root):
            dirs[:] = [d for d in dirs if d not in self.ignored_dirs]

            for file in files:
                if file.endswith(self.extensions):
                    full_path = os.path.join(root, file)
                    self.annotate_file(full_path)

        print("\n✅ Done (idempotent, no extra spaces).")


# ============================================================

if __name__ == "__main__":
    FilePathAnnotator().run()