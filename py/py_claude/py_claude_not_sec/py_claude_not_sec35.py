from typing import Dict, List, Optional, Type
from fastapi import FastAPI, HTTPException
from sqlalchemy import create_engine, MetaData
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import Session, sessionmaker
from pydantic import BaseModel, create_model
import inflect

# Initialisierung
app = FastAPI(title="Auto-Generated API")
p = inflect.engine()

# Datenbankverbindung konfigurieren
DATABASE_URL = "sqlite:///./test.db"  # Beispiel für SQLite
engine = create_engine(DATABASE_URL)
SessionLocal = sessionmaker(autocommit=False, autoload=True, bind=engine)
Base = declarative_base()
metadata = MetaData()

class APIGenerator:
    def __init__(self, db_url: str):
        self.engine = create_engine(db_url)
        self.metadata = MetaData()
        self.metadata.reflect(bind=self.engine)
        self.models: Dict[str, Type[BaseModel]] = {}
        
    def generate_pydantic_model(self, table_name: str) -> Type[BaseModel]:
        """Generiert ein Pydantic-Model aus einer Datenbanktabelle"""
        table = self.metadata.tables[table_name]
        fields = {}
        
        for column in table.columns:
            python_type = self._map_sql_to_python_type(column.type)
            fields[column.name] = (
                Optional[python_type] if column.nullable else python_type,
                ... if not column.nullable else None
            )
            
        model_name = f"{table_name.capitalize()}Model"
        return create_model(model_name, **fields)
    
    def _map_sql_to_python_type(self, sql_type: str) -> Type:
        """Mapping von SQL zu Python Datentypen"""
        type_mapping = {
            'INTEGER': int,
            'BIGINT': int,
            'SMALLINT': int,
            'VARCHAR': str,
            'TEXT': str,
            'BOOLEAN': bool,
            'FLOAT': float,
            'DECIMAL': float,
            'DATETIME': str,
            'DATE': str,
        }
        return type_mapping.get(sql_type.upper(), str)
    
    def generate_crud_routes(self, app: FastAPI, table_name: str):
        """Generiert CRUD-Routen für eine Tabelle"""
        model = self.generate_pydantic_model(table_name)
        self.models[table_name] = model
        table = self.metadata.tables[table_name]
        
        # GET all
        @app.get(f"/{table_name}/", response_model=List[model])
        def read_all(skip: int = 0, limit: int = 100, db: Session = Depends(get_db)):
            items = db.query(table).offset(skip).limit(limit).all()
            return [model.from_orm(item) for item in items]
        
        # GET one
        @app.get(f"/{table_name}/{{item_id}}", response_model=model)
        def read_one(item_id: int, db: Session = Depends(get_db)):
            item = db.query(table).filter(table.c.id == item_id).first()
            if item is None:
                raise HTTPException(status_code=404, detail="Item not found")
            return model.from_orm(item)
        
        # POST
        @app.post(f"/{table_name}/", response_model=model)
        def create(item: model, db: Session = Depends(get_db)):
            db_item = table.insert().values(**item.dict())
            db.execute(db_item)
            db.commit()
            return item
        
        # PUT
        @app.put(f"/{table_name}/{{item_id}}", response_model=model)
        def update(item_id: int, item: model, db: Session = Depends(get_db)):
            db_item = db.query(table).filter(table.c.id == item_id)
            if not db_item.first():
                raise HTTPException(status_code=404, detail="Item not found")
            db_item.update(item.dict())
            db.commit()
            return item
        
        # DELETE
        @app.delete(f"/{table_name}/{{item_id}}")
        def delete(item_id: int, db: Session = Depends(get_db)):
            db_item = db.query(table).filter(table.c.id == item_id)
            if not db_item.first():
                raise HTTPException(status_code=404, detail="Item not found")
            db_item.delete()
            db.commit()
            return {"message": "Item deleted"}

def get_db():
    """Datenbank-Session-Generator"""
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

# Verwendungsbeispiel
if __name__ == "__main__":
    # Beispiel-Datenbank erstellen
    Base.metadata.create_all(bind=engine)
    
    # API-Generator initialisieren
    generator = APIGenerator(DATABASE_URL)
    
    # APIs für alle Tabellen generieren
    for table_name in metadata.tables.keys():
        generator.generate_crud_routes(app, table_name)
    
    # Server starten
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)