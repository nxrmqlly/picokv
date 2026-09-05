pipeline "ci" {
    description = "ci for picokv"
}

checkout {
    url = "https://github.com/nxrmqlly/picokv",
    branch = "main",
}

job "build" {
    image = "silkeh/clang",

    run "make all"
}

job "sanitize" {
    image = "silkeh/clang",

    run "make san",
}
