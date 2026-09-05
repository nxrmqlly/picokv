pipeline "release" {
    description = "PicoKV release binaries",
}

checkout {
    url = "https://github.com/nxrmqlly/picokv",
    branch = "main",
}

local targets = {
    {
        name = "linux-amd64",
        packages = "clang make",
        command = "make clean all && mv dist/picokv dist/picokv-linux-amd64",
    },
    {
        name = "linux-arm64",
        packages = "make gcc-aarch64-linux-gnu",
        command = "make clean all CC=aarch64-linux-gnu-gcc TARGET=dist/picokv-linux-arm64",
    },
    {
        name = "windows-amd64",
        packages = "make gcc-mingw-w64-x86-64",
        command = "make clean all CC=x86_64-w64-mingw32-gcc TARGET=dist/picokv-windows-amd64.exe",
    },
}

local release_artifacts = {}

for _, target in ipairs(targets) do
    local job_name = "build-" .. target.name
    local artifact_name = "picokv-" .. target.name
    local output = "dist/" .. artifact_name

    if target.name == "windows-amd64" then
        output = output .. ".exe"
    end

    job(job_name) {
        image = "ubuntu:24.04",

        run "install toolchain" {
            cmd = "apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y " .. target.packages,
        },

        run "build" {
            cmd = target.command,
        },

        artifact("binary", output),
    }

    table.insert(release_artifacts, {
        job = job_name,
        name = "binary",
        as = artifact_name .. (target.name == "windows-amd64" and ".exe" or ""),
    })
end

github {
    release = {
        on = "published",
        artifacts = release_artifacts,
    },
}
