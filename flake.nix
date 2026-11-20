{
  description = "Python to RISC-V compiler: dev shell with the same tools as the Dockerfile";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
      riscv = pkgs.pkgsCross.riscv64;

      # tests/test.py and ./do expect the Debian/Ubuntu binary names
      gcc = pkgs.writeShellScriptBin "riscv64-linux-gnu-gcc" ''
        exec ${riscv.buildPackages.gcc}/bin/riscv64-unknown-linux-gnu-gcc \
          -L${riscv.glibc.static}/lib "$@"
      '';
      qemu = pkgs.writeShellScriptBin "qemu-riscv64-static" ''
        exec ${pkgs.qemu-user}/bin/qemu-riscv64 "$@"
      '';
    in {
      devShells.${system}.default = pkgs.mkShell {
        packages = [ pkgs.python312 gcc qemu ];
        PYTHONHASHSEED = "1";
      };
    };
}
