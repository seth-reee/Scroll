{ lib, stdenv, cmake, ninja, qt6, source, desktop-file-utils }:

stdenv.mkDerivation {
  pname = "scroll";
  version = builtins.elemAt (builtins.match ".*project\\(Scroll VERSION ([^ ]+) LANGUAGES.*"
    (builtins.readFile ../../CMakeLists.txt)) 0;
  src = lib.cleanSourceWith {
    src = source;
    filter = path: type:
      let name = baseNameOf path; in
      !(lib.hasInfix ".sync-conflict-" name)
      && !(builtins.elem name [ ".git" "build" "dist" "stage" "result" ])
      && !(lib.hasSuffix ".pkg.tar.zst" name)
      && !(lib.hasSuffix ".pkg.tar.xz" name);
  };
  nativeBuildInputs = [ cmake ninja qt6.wrapQtAppsHook ];
  buildInputs = [ qt6.qtbase qt6.qtwayland ];
  cmakeFlags = [ "-DBUILD_TESTING=ON" ];
  doCheck = true;
  preCheck = ''
    export HOME="$TMPDIR/home"
    mkdir -p "$HOME"
    export TMPDIR=/tmp
    export QT_QPA_PLATFORM=offscreen
    export QT_QPA_PLATFORMTHEME=
    export QT_QUICK_CONTROLS_STYLE=Basic
    export QT_QUICK_BACKEND=software
  '';
  nativeCheckInputs = [ desktop-file-utils ];
  postCheck = ''
    desktop-file-validate ../packaging/scroll.desktop
  '';


  meta = {
    description = "Lightweight Qt text editor";
    homepage = "https://github.com/seth-reee/Scroll";
    license = lib.licenses.mit;
    mainProgram = "scroll";
    platforms = [ "x86_64-linux" "aarch64-linux" ];
  };
}
