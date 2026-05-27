export const FAQ_QUESTIONS = [
  // 1. AI PRIORITIES (1-20)
  {
    category: "AI & Priority Rules",
    question: "How does s0ucipher AI decide what is High Priority?",
    answer: "s0ucipher scans file names for urgency tokens such as 'exam', 'final', 'test', 'important', 'grade', and 'submit'. Files matching these are immediately flagged as High Priority."
  },
  {
    category: "AI & Priority Rules",
    question: "What keywords trigger Medium priority classification?",
    answer: "Keywords like 'project', 'draft', 'homework', 'assignment', 'syllabus', and 'lecture' are classified as Medium Priority. They are important for current work but not immediate exam review."
  },
  {
    category: "AI & Priority Rules",
    question: "How are Low Priority files categorized?",
    answer: "Files that do not match academic urgency keywords (e.g. system files, logs, general photos, and archives) are marked as Low Priority."
  },
  {
    category: "AI & Priority Rules",
    question: "Can I manually change the priority of a file?",
    answer: "Yes, you can rename the file to include urgency words like 'important' or 'exam' to trigger high priority classification automatically, or override it in settings."
  },
  {
    category: "AI & Priority Rules",
    question: "Does s0ucipher read the contents of the files to prioritize them?",
    answer: "No, s0ucipher is designed to be 100% private. It prioritizes files using metadata, extension types, and name heuristics without opening file contents."
  },
  {
    category: "AI & Priority Rules",
    question: "What is the confidence score on priorities?",
    answer: "The confidence score reflects the system's certainty in its categorization, calculated based on exact token matches, recency, and file extension matching."
  },
  {
    category: "AI & Priority Rules",
    question: "Why are some PDFs marked as Low Priority?",
    answer: "A PDF is marked as Low Priority if it lacks academic keywords (e.g., 'invoice.pdf' or 'receipt.pdf'). If it contains keywords like 'notes.pdf' or 'chapter1.pdf', it gets High/Medium priority."
  },
  {
    category: "AI & Priority Rules",
    question: "How does the 'recency' of a file affect its priority score?",
    answer: "Files modified within the last 7 days are given a slight boost in confidence score because they are likely related to active college/work courses."
  },
  {
    category: "AI & Priority Rules",
    question: "Does s0ucipher prioritize folders or only individual files?",
    answer: "s0ucipher recursively parses folders to extract individual files, and each file is prioritized independently. Folders are not prioritized as single units."
  },
  {
    category: "AI & Priority Rules",
    question: "What does 'academic priority' mean?",
    answer: "It refers to the prioritization of study materials, lecture slides, assignments, and exam review sheets over random files."
  },
  {
    category: "AI & Priority Rules",
    question: "Why are study materials automatically given high priority?",
    answer: "To help college students immediately locate study guides, notes, and exam schedules without digging through cluttered directories."
  },
  {
    category: "AI & Priority Rules",
    question: "What happens if a filename contains both high and low priority terms?",
    answer: "If a filename matches any high-priority token (e.g. 'final_draft_archive'), the high priority rule takes precedence to ensure you don't miss critical files."
  },
  {
    category: "AI & Priority Rules",
    question: "Is there a list of priority rules I can modify?",
    answer: "The rule weights are hardcoded in the C backend's classifier logic. You can see how they are matching under `get_priority()` in organizer.c."
  },
  {
    category: "AI & Priority Rules",
    question: "How are image formats prioritized?",
    answer: "Image files are classified as 'Images' category and set to Low Priority unless they contain tokens like 'screenshot_lecture' or 'formula_cheat_sheet'."
  },
  {
    category: "AI & Priority Rules",
    question: "How are video formats prioritized?",
    answer: "Videos are placed in 'Videos' category and set to Low Priority by default, unless they have keywords indicating lecture recordings or tutorials."
  },
  {
    category: "AI & Priority Rules",
    question: "Does a file modified 5 years ago get marked as low priority?",
    answer: "Yes, files older than 180 days get flagged as 'cleanup-candidate' tags and are typically placed in Low Priority as legacy content."
  },
  {
    category: "AI & Priority Rules",
    question: "How are unknown extensions prioritized?",
    answer: "Unknown file types are categorized as 'Others' and given Low Priority unless their filenames match urgent academic keywords."
  },
  {
    category: "AI & Priority Rules",
    question: "What priority is given to programming source files (.c, .py, .java)?",
    answer: "Programming files are categorized under 'Study' and assigned Medium Priority by default because they represent homework/lab exercises."
  },
  {
    category: "AI & Priority Rules",
    question: "Why is 'exam_schedule' classified as High Priority?",
    answer: "Because it contains 'exam', which is a high-urgency keyword matching the core academic classification rules."
  },
  {
    category: "AI & Priority Rules",
    question: "How does s0ucipher distinguish between study materials and general office work?",
    answer: "By looking for academic tokens like 'assignment', 'syllabus', 'quiz', 'course', and 'lecture' versus general office terms."
  },

  // 2. FILE CATEGORIES & MATCHING (21-40)
  {
    category: "File Categories",
    question: "What categories does s0ucipher use for sorting?",
    answer: "s0ucipher categorizes files into 5 target folders: 'Study Hub' (academic files/code), 'Images', 'Documents' (PDFs/DOCX/TXT), 'Videos', and 'Others'."
  },
  {
    category: "File Categories",
    question: "How are programming projects categorized?",
    answer: "Code files (.c, .cpp, .py, .java, .js, .html) are placed in the 'Study Hub' category under 'Study Files' subdirectory."
  },
  {
    category: "File Categories",
    question: "Where do compressed archives (.zip, .tar, .rar) go?",
    answer: "All compressed archives and disk images (.zip, .tar, .gz, .rar, .7z, .dmg) are sorted into the 'Others' category."
  },
  {
    category: "File Categories",
    question: "How does s0ucipher classify PDF files?",
    answer: "If a PDF has study keywords, it goes to 'Study Hub'. Otherwise, it is classified as 'Documents'."
  },
  {
    category: "File Categories",
    question: "What extensions are recognized as Images?",
    answer: "Recognized image extensions include: .jpg, .jpeg, .png, .gif, .webp, .heic, and .svg."
  },
  {
    category: "File Categories",
    question: "What extensions are recognized as Videos?",
    answer: "Recognized video extensions include: .mp4, .mkv, .mov, .avi, and .webm."
  },
  {
    category: "File Categories",
    question: "What extensions are recognized as Documents?",
    answer: "Standard document extensions are: .pdf, .docx, .doc, .txt, .pptx, .xlsx, and .csv."
  },
  {
    category: "File Categories",
    question: "Where do audio files (.mp3, .wav) get sorted?",
    answer: "Audio files are classified as 'Others' because they don't map to primary categories (Photos, Videos, PDFs, or Study Hub)."
  },
  {
    category: "File Categories",
    question: "Can s0ucipher categorize folders themselves?",
    answer: "No, s0ucipher processes files. If you add a folder, it scans all files inside and sorts them into the 5 target category folders."
  },
  {
    category: "File Categories",
    question: "What happens to hidden system files (like .DS_Store or .gitignore)?",
    answer: "Hidden system files and configuration files are ignored by default to keep your organized workspace clean."
  },
  {
    category: "File Categories",
    question: "How does s0ucipher handle executable files (.exe, .app)?",
    answer: "Executables are categorized under 'Others' and flagged with standard tags to prevent accidental run issues."
  },
  {
    category: "File Categories",
    question: "Why is a file labeled 'Study_homework_1.py' sorted into Study Hub?",
    answer: "Because it contains the '.py' extension (code) and 'homework' keyword, matching the criteria for academic study."
  },
  {
    category: "File Categories",
    question: "Can I add custom file categories?",
    answer: "The 5 core categories are hardcoded in the C backend. To add new ones, you would need to adjust categorization rules in organizer.c."
  },
  {
    category: "File Categories",
    question: "How are font files (.ttf, .otf) classified?",
    answer: "Font files are categorized as 'Others' because they are system resources rather than user documents/study material."
  },
  {
    category: "File Categories",
    question: "Where do presentation slides (.pptx, .key) go?",
    answer: "Presentation files containing 'lecture', 'slides', or 'class' go to 'Study Hub'; others go to 'Documents'."
  },
  {
    category: "File Categories",
    question: "How does s0ucipher classify ebook formats (.epub, .mobi)?",
    answer: "Ebook files are classified as 'Documents' (unless they mention course study, in which case they go to Study Hub)."
  },
  {
    category: "File Categories",
    question: "Where do spreadsheets (.xlsx, .csv) go?",
    answer: "Spreadsheets go to the 'Documents' category, or 'Study Hub' if they contain homework/lab naming tokens."
  },
  {
    category: "File Categories",
    question: "How are markdown files (.md) classified?",
    answer: "Markdown files are treated as documents. If they are project READMEs, they go to Study Hub; otherwise, they go to Documents."
  },
  {
    category: "File Categories",
    question: "Are shortcuts (.lnk, .alias) organized?",
    answer: "No, shortcuts and alias files are ignored during scanning to prevent circular pointer traps."
  },
  {
    category: "File Categories",
    question: "How does s0ucipher handle database files (.db, .sql)?",
    answer: "Database files are classified under 'Others' unless they contain academic assignment keywords."
  },

  // 3. PRIVACY, SECURITY & OFFLINE (41-55)
  {
    category: "Privacy & Security",
    question: "Is my private data uploaded to any server?",
    answer: "No! All file analysis and local copy movements are executed directly on your device. Your data never leaves your computer."
  },
  {
    category: "Privacy & Security",
    question: "How does s0ucipher run without internet access?",
    answer: "The backend is written entirely in native C and runs a local server on port 5050. The frontend React app runs locally, making it fully offline-compatible."
  },
  {
    category: "Privacy & Security",
    question: "Does s0ucipher modify my original files?",
    answer: "By default, it only creates organized copies in 'Downloads/NeuroSort Organized' or your custom folder. Your original source files remain untouched."
  },
  {
    category: "Privacy & Security",
    question: "What security measures protect my local directories?",
    answer: "The server runs locally with your user permissions and does not accept remote network connections. External clients cannot access your files."
  },
  {
    category: "Privacy & Security",
    question: "Does s0ucipher install any tracking spyware?",
    answer: "No. s0ucipher contains no tracking scripts, telemetry, or third-party connections. You can inspect the source code to verify."
  },
  {
    category: "Privacy & Security",
    question: "Is it safe to run s0ucipher on system directories?",
    answer: "It is highly recommended to run it on specific folders (like Downloads or Documents) rather than system-wide roots (like /System or C:\\Windows)."
  },
  {
    category: "Privacy & Security",
    question: "How does s0ucipher ensure my files are not corrupted?",
    answer: "It uses standard POSIX C stream buffering and copy routines. Files are fully written and closed before verify checks are made."
  },
  {
    category: "Privacy & Security",
    question: "Can s0ucipher access my passwords or keychain?",
    answer: "No, s0ucipher only has access to the folders you select. It has no capability to read your system passwords or keychain data."
  },
  {
    category: "Privacy & Security",
    question: "Does s0ucipher require administrative/root privileges?",
    answer: "No, s0ucipher runs as a user-level process. It only needs write access to the directories you choose to organize."
  },
  {
    category: "Privacy & Security",
    question: "Can I use s0ucipher to encrypt my files?",
    answer: "No, s0ucipher is a sorting and classification utility. It does not encrypt, compress, or password-protect files."
  },
  {
    category: "Privacy & Security",
    question: "How does Web Sandbox Mode protect my device?",
    answer: "In Web Sandbox Mode, files are processed entirely in the browser's sandbox memory. The server only returns metadata, keeping your local disk untouched."
  },
  {
    category: "Privacy & Security",
    question: "Does s0ucipher scan files in my iCloud / cloud drive?",
    answer: "If you select your local iCloud sync folder, s0ucipher will scan the local copies. It does not make cloud API calls."
  },
  {
    category: "Privacy & Security",
    question: "Are duplicate check results saved anywhere?",
    answer: "No. Duplicate checks are run in-memory during the scan session and are cleared when you reset or close the application."
  },
  {
    category: "Privacy & Security",
    question: "Can s0ucipher scan my external USB drives?",
    answer: "Yes, you can choose directories on external volumes. The C backend will read and organize them normally."
  },
  {
    category: "Privacy & Security",
    question: "Does s0ucipher scan network shared folders?",
    answer: "Yes, as long as the network folder is mounted as a local mount point on your Mac system, it can be scanned."
  },

  // 4. C BACKEND & PERFORMANCE (56-70)
  {
    category: "C Backend Internals",
    question: "Why is the backend written in C instead of Python?",
    answer: "Your college requested a native C-based implementation with DSA algorithms. C compiles to a fast, low-memory binary with no runtime dependencies."
  },
  {
    category: "C Backend Internals",
    question: "How does the C multi-threaded socket server work?",
    answer: "The server uses `pthread` to create a separate detached thread for each incoming connection, ensuring requests do not block each other."
  },
  {
    category: "C Backend Internals",
    question: "What JSON library does the backend use?",
    answer: "The backend uses a lightweight, custom-written C parser (`json.c`) that compiles directly with the server code, avoiding external dependency bloat."
  },
  {
    category: "C Backend Internals",
    question: "How is sorting implemented in the C backend?",
    answer: "Sorting is implemented using a custom, stable Merge Sort algorithm. This maintains the relative order of files that have equal sort values."
  },
  {
    category: "C Backend Internals",
    question: "Is there a risk of memory leaks in the C backend?",
    answer: "No. The C code strictly tracks dynamically allocated buffers and implements cleanup routines to free memories after each client request."
  },
  {
    category: "C Backend Internals",
    question: "How fast is the directory scanning in C?",
    answer: "Extremely fast. C scans directories using low-level POSIX calls (`readdir` and `stat`), capable of indexing thousands of files in milliseconds."
  },
  {
    category: "C Backend Internals",
    question: "Does the C server support HTTP compression?",
    answer: "No, to keep the C codebase simple and maintainable, the server returns uncompressed HTTP responses, which is fast enough for local use."
  },
  {
    category: "C Backend Internals",
    question: "What compiler is used to build the C backend?",
    answer: "The backend is compiled using Clang on macOS (via Makefile), utilizing optimization flags like -O2 for high performance."
  },
  {
    category: "C Backend Internals",
    question: "How does the C backend handle non-ASCII filenames?",
    answer: "Filenames are processed as UTF-8 character arrays. The custom JSON writer escapes special bytes to prevent transmission formatting bugs."
  },
  {
    category: "C Backend Internals",
    question: "What is the max path limit for file paths in C?",
    answer: "The backend uses the standard POSIX `PATH_MAX` constant (usually 1024 or 4096 bytes depending on the platform) to allocate buffers safely."
  },
  {
    category: "C Backend Internals",
    question: "Why does the C backend use AppleScript?",
    answer: "AppleScript (`osascript`) is invoked via `popen` to trigger native macOS file and directory picker dialogs from a console C program."
  },
  {
    category: "C Backend Internals",
    question: "Can this C backend run on Windows?",
    answer: "No, the current C server is built for macOS and POSIX systems. It depends on Unix headers, POSIX threads, and macOS-specific AppleScripts."
  },
  {
    category: "C Backend Internals",
    question: "How does the C server serve static frontend files?",
    answer: "It opens static assets in `frontend/dist` in binary read mode (`rb`) and writes the raw content blocks directly to the client socket."
  },
  {
    category: "C Backend Internals",
    question: "What port does the backend run on?",
    answer: "By default, the backend runs on port 5050. This can be configured by setting the `PORT` environment variable before launching."
  },
  {
    category: "C Backend Internals",
    question: "How does the backend prevent infinite symbolic link loops?",
    answer: "It maintains a tracking array of canonical file paths. If a path matches an already scanned location, it skips it to prevent infinite loops."
  },

  // 5. SORTING & DIRECTORY STRUCTURING (71-85)
  {
    category: "Sorting & Structure",
    question: "What sorting methods are available?",
    answer: "You can sort files by Name, Size (descending), or Date Modified (newest first). This is configurable in the application settings."
  },
  {
    category: "Sorting & Structure",
    question: "What is the structure of the organized directory?",
    answer: "Inside the target directory, 5 category folders are created. Within each category, files are arranged by priority and name."
  },
  {
    category: "Sorting & Structure",
    question: "What happens if two files have the exact same name?",
    answer: "To prevent overwriting, s0ucipher appends a incrementing counter to duplicates, e.g. 'file.docx' and 'file (1).docx'."
  },
  {
    category: "Sorting & Structure",
    question: "Can I customize the folder names like 'Study Hub'?",
    answer: "The C backend has hardcoded targets. To customize, you would edit the `FOLDER_STUDY` constants in organizer.c and recompile."
  },
  {
    category: "Sorting & Structure",
    question: "Does s0ucipher delete original files after organizing?",
    answer: "No. s0ucipher makes copies of files. If 'applyChanges' is active in local mode, it moves files; otherwise, it keeps the originals."
  },
  {
    category: "Sorting & Structure",
    question: "What is the difference between ZIP export and Local Saving?",
    answer: "ZIP export downloads a .zip containing the categorized folders in the browser. Local Saving copies the directories directly onto your Mac's drive."
  },
  {
    category: "Sorting & Structure",
    question: "How does the name sorting algorithm work?",
    answer: "It performs case-insensitive ASCII string comparisons of file base names, placing folders and files in alphabetical order."
  },
  {
    category: "Sorting & Structure",
    question: "How does the date sorting algorithm work?",
    answer: "It compares the files' epoch time timestamps, placing the most recently modified files at the top of the preview table."
  },
  {
    category: "Sorting & Structure",
    question: "How does the size sorting algorithm work?",
    answer: "It compares file sizes in bytes, sorting them from largest to smallest, making it easy to identify large space-wasting files."
  },
  {
    category: "Sorting & Structure",
    question: "Can s0ucipher group files by date modified (e.g. Month/Year)?",
    answer: "No, s0ucipher groups files by functional category (Study Hub, Images, etc.). File timestamps are used strictly for table sorting."
  },
  {
    category: "Sorting & Structure",
    question: "Does s0ucipher preserve file creation dates?",
    answer: "When copying files, macOS usually sets the creation date to the copy date, but modification dates are preserved where possible."
  },
  {
    category: "Sorting & Structure",
    question: "Can I sort files in descending alphabetical order?",
    answer: "The standard name sort uses ascending alphabetical order. You can implement descending sort by editing the comparator inside organizer.c."
  },
  {
    category: "Sorting & Structure",
    question: "What happens to zero-byte empty files?",
    answer: "Empty files are categorized under 'Others' and flagged as 'empty-file' tags to let you know they contain no content."
  },
  {
    category: "Sorting & Structure",
    question: "Does s0ucipher support custom sorting rules based on tags?",
    answer: "Not currently. Sorting is applied by Name, Size, or Date. Smart tags are shown visually in the preview table but not used for sorting."
  },
  {
    category: "Sorting & Structure",
    question: "How does s0ucipher handle file paths with spaces?",
    answer: "The C backend surrounds all shell and AppleScript path arguments in single quotes to ensure spaces in folders are parsed correctly."
  },

  // 6. TROUBLESHOOTING & OPERATIONS (86-102)
  {
    category: "Troubleshooting",
    question: "Why does the scan show 'Scan connection failed'?",
    answer: "This means the React app cannot contact the backend on port 5050. Check that the C backend is compiled and running in the terminal."
  },
  {
    category: "Troubleshooting",
    question: "Why does folder scanning show 0 Bytes size?",
    answer: "If file access permission is missing, or if symbolic link loops are detected, files are skipped. We have fixed the realpath skip bug."
  },
  {
    category: "Troubleshooting",
    question: "How do I fix a permission denied error when organizing local folders?",
    answer: "Make sure you run the C server in a terminal that has permissions to access your directories, such as Terminal.app with Full Disk Access."
  },
  {
    category: "Troubleshooting",
    question: "Why is 'Choose Save Location' disabled in Web Sandbox Mode?",
    answer: "Because browser sandbox environments cannot write directly to custom local paths due to security. Use 'Save to Downloads' instead."
  },
  {
    category: "Troubleshooting",
    question: "How do I restart the backend server on my Mac?",
    answer: "Kill the process using Ctrl+C in your terminal, run `make clean && make` to rebuild, and then launch it using `./server`."
  },
  {
    category: "Troubleshooting",
    question: "What should I do if the AppleScript file dialog does not show up?",
    answer: "Ensure your terminal has permission to control System Events. Go to System Settings > Privacy & Security > Automation."
  },
  {
    category: "Troubleshooting",
    question: "Why are my files not moving after clicking 'Export & Save Copy'?",
    answer: "Check that the destination path is writable. If you are in Web Mode, it downloads a ZIP instead of moving files on the disk."
  },
  {
    category: "Troubleshooting",
    question: "Can I cancel a running scan?",
    answer: "Yes, you can click the 'Pause' button in the workspace bar to halt the scan, or click 'Reset' to clear the current session."
  },
  {
    category: "Troubleshooting",
    question: "How do I change the default port from 5050?",
    answer: "Run `export PORT=8080` in your terminal shell before executing `./server`, and update VITE_API_BASE_URL accordingly."
  },
  {
    category: "Troubleshooting",
    question: "What should I do if the frontend page shows a blank page?",
    answer: "Ensure you have compiled the frontend using `npm run build` so that the C server can find files inside the `frontend/dist` directory."
  },
  {
    category: "Troubleshooting",
    question: "How can I check if the server is healthy?",
    answer: "Open `http://localhost:5050/health` in your browser. It should return a JSON response: `{\"status\":\"ok\",\"app\":\"NeuroSort AI\"}`."
  },
  {
    category: "Troubleshooting",
    question: "Why are some files shown in 'Others' category?",
    answer: "If a file doesn't match primary document, study, image, or video extensions, s0ucipher places it in 'Others' to be safe."
  },
  {
    category: "Troubleshooting",
    question: "Why are duplicate warnings appearing?",
    answer: "This happens when files with the exact same name are scanned. Check the 'Cleanup Suggestions' panel to see where the duplicates are."
  },
  {
    category: "Troubleshooting",
    question: "How do I clear the cached selection in the app?",
    answer: "Click the 'Reset' button in the sidebar under the selected files summary to clear all file list caches."
  },
  {
    category: "Troubleshooting",
    question: "Can I run multiple instances of s0ucipher server?",
    answer: "You can, but they must bind to different port numbers to avoid network socket binding conflicts."
  },
  {
    category: "Troubleshooting",
    question: "Does s0ucipher log my file names?",
    answer: "No. The C backend prints connection logs and request status, but does not print or save your filenames on any external log files."
  },
  {
    category: "Troubleshooting",
    question: "What should I do if a large video file crashes the scan?",
    answer: "Scanning is metadata-only, so file size does not impact scan stability. It is safe to scan files of any size."
  }
];
