docker build -f jenkins/Dockerfile.dev -t tritoncts .
docker run -v $(pwd):/tritoncts tritoncts bash -c "./tritoncts/jenkins/install.sh"