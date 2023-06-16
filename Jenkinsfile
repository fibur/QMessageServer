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

                    dir('/data/data/com.termux/files/home/jenkins/') {
		                sh "JENKINS_NODE_COOKIE=dontKillMe ./run_server.sh" 
                    }
                }
            }
        }
    }
}