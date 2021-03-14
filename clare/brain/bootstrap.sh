# free up some space
sudo apt-get purge wolfram-engine
sudo apt-get purge libreoffice
sudo apt-get clean
sudo apt-get autoremove

# install updates
sudo apt-get update && sudo apt-get upgrade

# setup git
git config --global user.email "dgorissen@gmail.com"
git config --global user.name "Dirk Gorissen"

# get std packages
sudo apt install -y vim

# install docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker dgorissen

