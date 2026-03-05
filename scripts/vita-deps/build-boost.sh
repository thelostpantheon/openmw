#!/bin/bash
# Build/install Boost for OpenMW Vita port
# Uses vitasdk/packages if available, otherwise builds from source
set -e

VITASDK="${VITASDK:-/usr/local/vitasdk}"
SYSROOT="${VITASDK}/arm-vita-eabi"

echo "=== Installing Boost for Vita ==="

# Check if vdpm is available
if command -v vdpm &> /dev/null; then
    echo "Using vdpm to install Boost..."
    vdpm boost
    echo "=== Boost installed via vdpm ==="
    exit 0
fi

# Manual build fallback
WORKDIR="${1:-$(pwd)/vita-deps-build/boost}"
BOOST_VERSION="1.78.0"
BOOST_VERSION_UNDERSCORE="1_78_0"

echo "Building Boost ${BOOST_VERSION} from source..."
echo "Work dir: ${WORKDIR}"

mkdir -p "${WORKDIR}"
cd "${WORKDIR}"

if [ ! -f boost_done ]; then
    # Download
    if [ ! -f "boost_${BOOST_VERSION_UNDERSCORE}.tar.bz2" ]; then
        wget -q "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_UNDERSCORE}.tar.bz2"
    fi

    tar xf "boost_${BOOST_VERSION_UNDERSCORE}.tar.bz2"
    cd "boost_${BOOST_VERSION_UNDERSCORE}"

    # Create user-config.jam for Vita cross-compilation
    cat > user-config.jam << 'EOF'
using gcc : vita : arm-vita-eabi-g++ ;
EOF

    # Bootstrap b2
    ./bootstrap.sh

    # Build only the libraries OpenMW needs
    ./b2 \
        --user-config=user-config.jam \
        toolset=gcc-vita \
        target-os=linux \
        runtime-link=static \
        link=static \
        threading=multi \
        variant=release \
        --with-iostreams \
        --with-program_options \
        --with-system \
        --with-filesystem \
        --prefix="${SYSROOT}" \
        install

    cd ..
    touch boost_done
fi

echo "=== Boost installation complete ==="
