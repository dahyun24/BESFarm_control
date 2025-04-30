-- sensor 테이블 생성
CREATE TABLE IF NOT EXISTS sensor (
  id INT AUTO_INCREMENT PRIMARY KEY,
  temp DECIMAL(5,2),   
  humidity DECIMAL(5,2),
  co2 INT UNSIGNED,
  date DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- control_rule 테이블 생성
CREATE TABLE IF NOT EXISTS control_rule (
    id INT AUTO_INCREMENT PRIMARY KEY,
    fan VARCHAR(10),
    window VARCHAR(10),
    cooling VARCHAR(10),
    heating VARCHAR(10),
    date DATETIME DEFAULT CURRENT_TIMESTAMP
);
