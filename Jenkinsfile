pipeline {
    agent any

    stages {
        stage('Build') {
            steps {
                dir('build') {
                    sh 'cmake -DCMAKE_BUILD_TYPE=Release ..'
                    sh 'cmake --build .'
                }
            }
        }
        stage('Install') {
            when {
                branch 'master'
            }
            steps {
                script {
                    dir('build') {
                        sh 'cmake --install .'
                    }
		    sh "JENKINS_NODE_COOKIE=dontKillMe /data/data/com.termux/files/home/jenkins/run_server.sh"
                }
            }
        }
    }
}